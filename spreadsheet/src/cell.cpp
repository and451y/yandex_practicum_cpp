#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>


// Реализуйте следующие методы
Cell::Cell(Sheet& sheet) : sheet_(sheet) {
}

Cell::~Cell() {
}


void Cell::Set(std::string text) {
	if (text.empty()) {
		impl_ = std::make_unique<Impl>(EmptyImpl());
	}
	else {
		char first_c = text.front();

		if (first_c == '\'') {
			impl_ = std::make_unique<Impl>(TextImpl(text));
		}
		else if (first_c == '=' && text.size() > 1) {
			impl_ = std::make_unique<Impl>(FormulaImpl(text.substr(1)));
		}
		else {
			impl_ = std::make_unique<Impl>(TextImpl(text));
		}
	}
}

void Cell::Clear() {
	impl_.reset(nullptr);
}

Cell::Value Cell::GetValue() const {
	if (cache_.has_value()) {
		return cache_.value();
	}

	if (std::holds_alternative<std::string>(impl_.get()->GetValue())) {
		std::string result = std::get<std::string>(impl_.get()->GetValue());
		if (result.empty())
			return {};

		if (result.front() == '\'')
			return result.substr(1);

		return result;
	}

	if (std::holds_alternative<std::unique_ptr<FormulaInterface>>(impl_.get()->GetValue())) {
		SheetInterface& si = reinterpret_cast<SheetInterface&>(sheet_);

		std::variant<double, FormulaError> foo = std::get<std::unique_ptr<FormulaInterface>>(impl_.get()->GetValue())->Evaluate(si);

		if (std::holds_alternative<double>(foo)) {
			auto& cache_ref = GetMutableCache();
			cache_ref = { std::get<double>(foo) };
			return std::get<double>(foo);
		}

		if (std::holds_alternative<FormulaError>(foo))
			return std::get<FormulaError>(foo);
	}

	return {};
}

std::string Cell::GetText() const {
	if (std::holds_alternative<std::string>(impl_.get()->GetValue())) {
		std::string result = std::get<std::string>(impl_.get()->GetValue());
		return result;
	}

	if (std::holds_alternative<std::unique_ptr<FormulaInterface>>(impl_.get()->GetValue())) {
		return "=" + std::get<std::unique_ptr<FormulaInterface>>(impl_.get()->GetValue()).get()->GetExpression();
	}

	return {};
}

std::vector<Position> Cell::GetReferencedCells() const {
	std::vector<Position> result;

	if (std::holds_alternative<std::unique_ptr<FormulaInterface>>(impl_.get()->GetValue())) {
		result = std::move(std::get<std::unique_ptr<FormulaInterface>>(impl_.get()->GetValue()).get()->GetReferencedCells());
	}
	
	return result;
}

bool Cell::IsReferenced() const {
	return !GetReferencedCells().empty();
}

std::optional<double>& Cell::GetMutableCache() const {
	return cache_;
}


