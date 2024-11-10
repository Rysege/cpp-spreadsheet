#pragma once

#include "cell.h"
#include "common.h"

#include <iostream>
#include <unordered_map>

struct Hasher {
    size_t operator()(Position pos) const {
        return std::hash<int>{}(pos.row) + std::hash<int>{}(pos.col) * 37;
    }
};


class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;


private:
    using CellPtr = std::unique_ptr<Cell>;
    std::unordered_map<Position, CellPtr, Hasher> cells_;

    template<class Func>
    void Print(Func func, std::ostream& output) const;
};

template<class Func>
inline void Sheet::Print(Func func, std::ostream& output) const {
    auto [rows, cols] = GetPrintableSize();

    for (int row = 0; row < rows; ++row) {
        std::string sep;
        for (int col = 0; col < cols; ++col) {
            output << sep, sep = '\t';
            if (auto cell = GetCell({ row, col })) {
                func(cell);
            }
        }
        output << '\n';
    }

}
