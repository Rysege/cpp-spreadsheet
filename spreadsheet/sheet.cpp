#include "sheet.h"

#include <algorithm>

using namespace std::literals;

Sheet::~Sheet() = default;

void CheckPosition(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position " + pos.ToString());
    }
}

void Sheet::SetCell(Position pos, std::string text) {
    CheckPosition(pos);

    auto [item, _] = cells_.try_emplace(pos, std::make_unique<Cell>(*this));
    item->second->Set(std::move(text));
}

const CellInterface* Sheet::GetCell(Position pos) const {
    return const_cast<Sheet&>(*this).GetCell(pos);
}

CellInterface* Sheet::GetCell(Position pos) {
    CheckPosition(pos);

    auto it = cells_.find(pos);
    return it == cells_.end() ? nullptr : it->second.get();
}

void Sheet::ClearCell(Position pos) {
    CheckPosition(pos);

    auto it = cells_.find(pos);
    if (it != cells_.end()) {
        it->second->Clear();

        if (!it->second->IsReferenced()) {
            cells_.erase(it);
        }
    }
}

Size Sheet::GetPrintableSize() const {
    if (cells_.empty()) {
        return {};
    }

    int row = 0;
    int col = 0;
    for (const auto& [pos, cell] : cells_) {
        if (cell->IsReferenced() || cell->GetText() != "") {
            row = std::max(row, pos.row);
            col = std::max(col, pos.col);
        }
    }
    return { row + 1, col + 1 };
}

void Sheet::PrintValues(std::ostream& output) const {
    auto print_value = [&output](const CellInterface* cell) {
        std::visit([&output](const auto& value) {output << value; }, cell->GetValue());
    };
    Print(print_value, output);
}

void Sheet::PrintTexts(std::ostream& output) const {
    auto print_text = [&output](const CellInterface* cell) {
        output << cell->GetText();
    };
    Print(print_text, output);
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}