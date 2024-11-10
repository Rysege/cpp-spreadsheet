#include "formula.h"
#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

FormulaError::FormulaError(FormulaError::Category category)
    : category_(category) {
}

FormulaError::Category FormulaError::GetCategory() const {
    return category_;
}

bool FormulaError::operator==(FormulaError rhs) const {
    return category_ == rhs.category_;
}

std::string_view FormulaError::ToString() const {
    switch (category_) {
    case FormulaError::Category::Ref:
        return "#REF!";
    case FormulaError::Category::Value:
        return "#VALUE!";
    case FormulaError::Category::Arithmetic:
        return "#ARITHM!";
    default:
        return "";
    }
}

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

double AsDouble(const std::string& str) {
    if (str.empty()) {
        return 0.;
    }

    std::istringstream iss(str);
    double value;
    if (iss >> value && iss.eof()) {
        return value;
    }

    throw FormulaError(FormulaError::Category::Value);
}

double AsDouble(double val) {
    return val;
}

double AsDouble(const FormulaError& fer) {
    throw fer;
}

namespace {
class Formula : public FormulaInterface {
public:
    explicit Formula(const std::string& expression) try
        : ast_(ParseFormulaAST(expression)) {
    }
    catch (const std::exception&) {
        throw FormulaException("There is a problem with this formula");
    }
    
    Value Evaluate(const SheetInterface& sheet) const override {
        auto args = [&sheet](Position pos) {
            if (auto cell = sheet.GetCell(pos)) {
                return std::visit([](const auto& x) { return AsDouble(x); }, cell->GetValue());
            }
            return 0.;
        };

        try {
            return ast_.Execute(args);
        }
        catch (const FormulaError& exc) {
            return exc;
        }
    }

    std::string GetExpression() const override {
        std::ostringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        auto cells = ast_.GetCells();
        cells.unique();
        return std::vector<Position>(std::make_move_iterator(cells.begin()),
                                     std::make_move_iterator(cells.end()));
    }
private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}