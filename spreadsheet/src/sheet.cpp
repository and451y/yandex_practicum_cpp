#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
	if (!pos.IsValid())
		throw InvalidPositionException("invalid position");

	auto cell = std::make_unique<Cell>(*this);
	cell.get()->Set(text);

	if (!CheckCircularRef(pos, cell.get()))
		throw CircularDependencyException("CircularDependencyException"s);

	sheet_[pos] = std::move(cell);
	CheckPrintSize();
	ClearCache();
}

const CellInterface* Sheet::GetCell(Position pos) const {
	if (!pos.IsValid())
		throw InvalidPositionException("invalid position");

	auto result = sheet_.find(pos);

	if (pos.row < print_size_.rows && pos.col < print_size_.cols ) {
		if (result == sheet_.end()) {
			return empty_cell_.get();
		}
		else {
			return result->second.get();
		}
	}

	return nullptr;
}

CellInterface* Sheet::GetCell(Position pos) {
	if (!pos.IsValid())
		throw InvalidPositionException("invalid position");

	auto result = sheet_.find(pos);

	if (pos.row < print_size_.rows && pos.col < print_size_.cols) {
		if (result == sheet_.end()) {
			return empty_cell_.get();
		}
		else {
			return result->second.get();
		}
	}

	return nullptr;

}

void Sheet::ClearCell(Position pos) {
	if (!pos.IsValid())
		throw InvalidPositionException("invalid position");

	if (sheet_.count(pos)) {
		sheet_.erase(pos);
		CheckPrintSize();
	}

}

Size Sheet::GetPrintableSize() const {
	return print_size_;
}

void Sheet::PrintValues(std::ostream& output) const {
	for (int row = 0; row < print_size_.rows; ++row) {
		for (int col = 0; col < print_size_.cols; ++col) {
			Position pos = {row, col};
			const CellInterface* cell = GetCell(pos);

			if (std::holds_alternative<std::string>(cell->GetValue())) {
				output << std::get<std::string>(cell->GetValue());
			} else if (std::holds_alternative<double>(cell->GetValue())) {
				output << std::get<double>(cell->GetValue());
			} else if (std::holds_alternative<FormulaError>(cell->GetValue())) {
				output << std::get<FormulaError>(cell->GetValue());
			}

			if (col != print_size_.cols - 1)
				output << "\t"sv;

		}

		output << "\n"sv;
	}
}
void Sheet::PrintTexts(std::ostream& output) const {
	for (int row = 0; row < print_size_.rows; ++row) {
		for (int col = 0; col < print_size_.cols; ++col) {
			Position pos = { row, col };
			const CellInterface* cell = GetCell(pos);

			if (col != print_size_.cols - 1) {
					output << cell->GetText() << "\t"sv;
			}
			else {
					output << cell->GetText();
			}
		}

		output << "\n"sv;
	}
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}

void Sheet::ClearCache() {
	for (auto& cell : sheet_) {
		cell.second.get()->GetMutableCache() = {};
	}
}

void Sheet::CheckPrintSize() {
	int last_non_empty_col = 0;
	int last_non_empty_row = 0;

	for (const auto& [pos, _] : sheet_) {
		last_non_empty_col = std::max(pos.col + 1, last_non_empty_col);
		last_non_empty_row = std::max(pos.row + 1, last_non_empty_row);
	}

	print_size_.rows = last_non_empty_row;
	print_size_.cols = last_non_empty_col;
}

bool Sheet::CheckCircularRef(const Position check_pos, const CellInterface* cell) const {
	if (cell == nullptr)
		return true;

	std::vector<Position> ref_cell_pos = cell->GetReferencedCells();

	if (ref_cell_pos.empty())
		return true;

	for (const auto& pos : ref_cell_pos) {
		if (check_pos == pos)
			return false;

		if (!CheckCircularRef(check_pos, GetCell(pos)))
			return false;
	}

	return true;
}

