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
    explicit Formula(std::string expression) : ast_(ParseFormulaAST(std::move(expression))) {}

    Value Evaluate(const SheetInterface& sheet) const override {

        std::function<double(Position)> functional_cell = [&sheet](Position pos) {
            double value = 0.0;

            if (!pos.IsValid()) {
                throw FormulaError(FormulaError::Category::Ref);
            }

            if (sheet.GetCell(pos) == nullptr) {
                return 0.0; 
            }

            CellInterface::Value value_ = sheet.GetCell(pos)->GetValue();

            if (std::holds_alternative<double>(value_)) {
                return std::get<double>(value_);
            }

            if (std::holds_alternative<FormulaError>(value_)) {
                throw std::get<FormulaError>(value_);
            }

            if (std::holds_alternative<std::string>(value_)) {
                try {
                    return std::stod(std::get<std::string>(value_));
                }
                catch (std::invalid_argument const& ex) {
                    throw FormulaError(FormulaError::Category::Value);
                }
            }
            return value;
        };

        try {
            return ast_.Execute(functional_cell);
        }
        catch (const FormulaError& fe) {
            return fe;
        }
    }

    std::string GetExpression() const override {
        std::ostringstream ost;
        ast_.PrintFormula(ost);
        return ost.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        std::set<Position> referenced_cells_;

        for (const auto& ref : ast_.GetCells()) {
            referenced_cells_.insert(ref);
        }
        std::vector<Position> result = { referenced_cells_.begin(), referenced_cells_.end() };
        return result;
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try {
        return std::make_unique<Formula>(std::move(expression));
    } 
    catch (...) {
            throw FormulaException("incorrect formula");
    }
}