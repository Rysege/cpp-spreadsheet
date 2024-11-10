#pragma once

#include "common.h"
#include "formula.h"

#include <unordered_set>


class Cell : public CellInterface {
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

public:
    Cell(SheetInterface& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();
    bool IsReferenced() const;

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

private:
    std::unique_ptr<Impl> impl_;
    SheetInterface& sheet_;
    std::unordered_set<Cell*> exploited_; // содержит ячейки, которые ссылаются на ячейку

    // очищает кэш у себя и у ячеек, которые ссылаются на эту ячейку
    void ClearCache();
    // ищет циклические ссылки, которые напрямую или косвенно ссылаются на самих себя
    bool HasCyclicDependencies(Impl* impl);
    // удаляет ссылку на себя у ячеек, на которые ссылалась
    // если ячейка, на которую ссылалась, является пустой и на неё ни кто не ссылается, она удаляется
    void DeleteDependencies();
    // добавляет ссылки на себя у ячеек, на которые ссылается эта ячейка
    void AddDependencies();
};