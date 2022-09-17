#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << "#DIV/0!";
}

namespace {
class Formula : public FormulaInterface {
public:
    explicit Formula(std::string expression) : ast_(ParseFormulaAST(expression)) {
    }

    Value Evaluate(const SheetInterface& sheet) const override {
    	Value result;

        std::function<double(Position*)> function = [&sheet](Position* pos) {
            double result = 0;
            const CellInterface* cell = sheet.GetCell(*pos);


            if (cell != nullptr) {
                std::variant<std::string, double, FormulaError> value = cell->GetValue();

                if (std::holds_alternative<double>(value)) {
                    result = std::get<double>(value);
                }

                if (std::holds_alternative<std::string>(value)) {
                    std::string temp = std::get<std::string>(value);

                    if (!temp.empty()) {
                        for (const auto& c : temp) {
                            if (!(c >= '1' && c <= '9'))
                                 throw FormulaError(FormulaError::Category::Value);
                        }

                        result = std::stod(temp);
                    }
                }
            }


            return result; 
        };

    	try {
    		result = ast_.Execute(function);
		} catch (FormulaError& e) {
			result = e;
		}

		return result;
    }

    std::string GetExpression() const override {
    	std::stringstream out;
    	ast_.PrintFormula(out);
    	return out.str();
    }

    std::vector<Position> GetReferencedCells() const {
        std::set<Position> temp = { ast_.GetCells().begin(), ast_.GetCells().end() };

        return { temp.begin(), temp.end() };
    };


private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    std::unique_ptr<FormulaInterface> result;

    try
    {
        result = std::make_unique<Formula>(std::move(expression));
    }
    catch (const std::exception&)
    {
        throw FormulaException("incorrect reference cell"s);
    }

    return result;
}

