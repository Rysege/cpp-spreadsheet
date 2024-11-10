#include "cell.h"

#include <cassert>
#include <iostream>
#include <optional>
#include <stack>
#include <string>

class Cell::Impl {
public:
    virtual ~Impl() = default;
    virtual Value GetValue() const = 0;
    virtual std::string GetText() const = 0;
    virtual std::vector<Position> GetReferencedCells() const { return {}; }
    virtual void ResetCache() {}
    virtual bool HasCache() const { return false; }
};

class Cell::EmptyImpl : public Impl {
public:
    Value GetValue() const override {
        return Value();
    }
    std::string GetText() const override {
        return std::string();
    }
};

class Cell::TextImpl : public Impl {
public:
    TextImpl(std::string text)
        : text_(std::move(text)) {
    }
    Value GetValue() const override {
        return text_[0] == ESCAPE_SIGN ? text_.substr(1) : text_;
    }
    std::string GetText() const override {
        return text_;
    }

private:
    std::string text_;
};

class Cell::FormulaImpl : public Impl {
public:
    FormulaImpl(std::string text, SheetInterface& sheet)
        : formula_(ParseFormula(std::move(text)))
        , sheet_(sheet) {
        referenced_cells_ = formula_->GetReferencedCells();
    }

    Value GetValue() const override {
        if (!cache_) {
            cache_.emplace(formula_->Evaluate(sheet_));
        }

        return std::visit([](const auto& x) { return Value(x); }, *cache_);
    }

    std::string GetText() const override {
        return FORMULA_SIGN + formula_->GetExpression();
    }

    std::vector<Position> GetReferencedCells() const override {
        return referenced_cells_;
    }

    void ResetCache() override {
        cache_.reset();
    }

    bool HasCache() const override {
        return cache_.has_value();
    }

private:
    std::unique_ptr<FormulaInterface> formula_;
    SheetInterface& sheet_;
    mutable std::optional<FormulaInterface::Value> cache_;
    // Содержит ячейки, которые задействованы в данной формуле.
    // Список отсортирован и не содержит повторяющихся ячеек.
    std::vector<Position> referenced_cells_; 
};

Cell::Cell(SheetInterface& sheet)
    : impl_(std::make_unique<EmptyImpl>())
    , sheet_(sheet) {
}

Cell::~Cell() = default;

void Cell::Set(std::string text) {
    std::unique_ptr<Impl> impl;

    if (text.empty()) {
        impl = std::make_unique<EmptyImpl>();
    }
    else if (text.size() > 1 && text[0] == FORMULA_SIGN) {
        impl = std::make_unique<FormulaImpl>(text.substr(1), sheet_);

        if (HasCyclicDependencies(impl.get())) {
            throw CircularDependencyException("Formula contain a circular reference");
        }
    }
    else {
        impl = std::make_unique<TextImpl>(std::move(text)); 
    }

    ClearCache();
    DeleteDependencies();
    impl_ = std::move(impl);
    AddDependencies();
}

void Cell::Clear() {
    Set("");
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

bool Cell::IsReferenced() const {
    return !exploited_.empty();
}

void Cell::ClearCache() {
    impl_->ResetCache();
    for (auto cell : exploited_) {
        if (cell->impl_->HasCache()) {
            cell->ClearCache();
        }
    }
}

void Cell::AddDependencies() {
    for (auto pos : GetReferencedCells()) {
        auto ptr = sheet_.GetCell(pos);
        if (!ptr) {
            sheet_.SetCell(pos, "");
            ptr = sheet_.GetCell(pos);
        }
        if (auto cell = dynamic_cast<Cell*>(ptr)) {
            cell->exploited_.insert(this);
        }
    }
}

void Cell::DeleteDependencies() {
    for (auto pos : GetReferencedCells()) {
        if (auto cell = dynamic_cast<Cell*>(sheet_.GetCell(pos))) {
            cell->exploited_.erase(this);
            if (dynamic_cast<EmptyImpl*>(cell->impl_.get())) {
                sheet_.ClearCell(pos);
            }
        }
    }
}

bool Cell::HasCyclicDependencies(Impl* impl) {
    std::unordered_set<Cell*> used;
    std::stack<Impl*> queue;
    queue.push(impl);

    while (!queue.empty()) {
        auto curr_impl = queue.top();
        queue.pop();

        for (auto pos : curr_impl->GetReferencedCells()) {
            if (auto cell_ptr = dynamic_cast<Cell*>(sheet_.GetCell(pos))) {
                if (used.emplace(cell_ptr).second) {
                    if (cell_ptr == this) {
                        return true;
                    }
                    queue.push(cell_ptr->impl_.get());
                }
            }
        }
    }
    return false;
}
