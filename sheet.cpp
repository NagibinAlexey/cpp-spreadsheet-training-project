#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::ResizeToPosition(Position pos) {
    if (pos.row > size_.rows - 1) {
        sheet_.resize(pos.row + 1);
        for (int i = size_.rows; i < pos.row + 1; ++i) {
            sheet_[i].resize(size_.cols);
        }
        size_.rows = pos.row + 1;
        printable_size_.rows = pos.row + 1;
    }

    if (pos.col > size_.cols - 1) {
        for (auto& row : sheet_) {
            row.resize(pos.col + 1);
        }
        size_.cols = pos.col + 1;
        printable_size_.cols = pos.col + 1;
    }
}

void Sheet::SetDependences(const Position pos) {
    for (const auto& ref_pos : GetCell(pos)->GetReferencedCells()) {
        if (ref_pos.row > size_.rows - 1 || ref_pos.col > size_.cols - 1) {
            ResizeToPosition(ref_pos);
        }
        if (sheet_[ref_pos.row][ref_pos.col] == nullptr) {
            auto new_cell_ = std::make_unique<Cell>(*this);
            new_cell_->Set("");
            sheet_[ref_pos.row][ref_pos.col] = std::move(new_cell_);
        }
        sheet_[ref_pos.row][ref_pos.col]->AddDependencedCell(pos);
    }
}

void Sheet::CreateNewCell(const Position pos, std::string text) {
    auto cell_ = std::make_unique<Cell>(*this);
    cell_->Set(text);

    Position temp_max_pos = { size_.rows, size_.cols };

    sheet_[pos.row][pos.col] = std::move(cell_);

    if (HasCircularDependency(pos)) {
        cell_ = nullptr;
        ResizeToPosition(temp_max_pos);
        throw CircularDependencyException("circular dependency");
    }

    ResetCache(pos);
    SetDependences(pos);
}

void Sheet::EditCell(const Position pos, std::string text) {
    auto cell_ = GetConcreteCell(pos);
    if (cell_->GetText() == text) return;

    std::string temp_text_ = cell_->GetText();
    cell_->Set(text);

    if (HasCircularDependency(pos)) {
        cell_->Set(temp_text_);
        throw CircularDependencyException("circular dependency");
    }

    ResetCache(pos);
    
    ClearDependences(pos);
    SetDependences(pos);
}

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("incorrect position");
    }

    if (pos.row > size_.rows - 1 || pos.col > size_.cols - 1) {
        ResizeToPosition(pos);
    }

    if (sheet_[pos.row][pos.col] == nullptr) {
        CreateNewCell(pos, text);
    }
    else {
        EditCell(pos, text);
    } 
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("incorrect position");
    }

    if (pos.row > size_.rows - 1 || pos.col > size_.cols - 1) {
        return nullptr;
    }

    return sheet_[pos.row][pos.col].get();
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("incorrect position");
    }

    if (pos.row > size_.rows - 1 || pos.col > size_.cols - 1) {
        return nullptr;
    }

    return sheet_[pos.row][pos.col].get();
}

const Cell* Sheet::GetConcreteCell(Position pos) const {
    return GetConcreteCell(pos);
}

Cell* Sheet::GetConcreteCell(Position pos) {
    CellInterface* cell_interface_ptr = GetCell(pos);
    return static_cast<Cell*>(cell_interface_ptr);
}

void Sheet::ResetCache(Position start_pos) {
    auto cell = GetConcreteCell(start_pos);
    if (cell == nullptr) return;

    if (cell->IsCached()) {
        cell->ResetCacheToNull();
        for (const auto& deps_pos : cell->GetDependencedCells()) {
            ResetCache(deps_pos);
        }
    }
}

void Sheet::HasCircularDependencyImpl(const Position pos, const Position init_pos, bool& has_circular) const {
    std::vector<Position> ref_cells{};
    if (GetCell(pos) != nullptr) { ref_cells = GetCell(pos)->GetReferencedCells(); }
    for (const auto& pos_ : ref_cells) {
        if (pos_ == init_pos) {
            has_circular = true;
            return;
        }
        HasCircularDependencyImpl(pos_, init_pos, has_circular);
    }
    return;
}

bool Sheet::HasCircularDependency(const Position pos) const {
    bool has_circular_ = false;
    HasCircularDependencyImpl(pos, pos, has_circular_);
    return has_circular_;
}

void Sheet::UpdatePrintableSize() {
    Position new_size{};
    bool found = false;
    for (int i = printable_size_.rows - 1; i >= 0; --i) {
        for (int j = 0; j < printable_size_.cols; ++j) {
            if (sheet_[i][j] != nullptr) {
                new_size.row = i + 1;
                found = true;
                break;
            }
        }
        if (found == true) break;
    }
    found = false;

    for (int j = printable_size_.cols - 1; j >= 0; --j) {
        for (int i = 0; i < printable_size_.rows; ++i) {
            if (sheet_[i][j] != nullptr) {
                new_size.col = j + 1;
                found = true;
                break;
            }
        }
        if (found == true) break;
    }
    printable_size_.rows = new_size.row;
    printable_size_.cols = new_size.col;
}

void Sheet::ClearDependences(Position pos) {
    auto cell_ = GetConcreteCell(pos);
    for (const auto& deps_pos : cell_->GetDependencedCells()) {
        auto dep_cell = GetConcreteCell(deps_pos);
        dep_cell->RemoveDependencedCell(pos);
    }
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("incorrect position");
    }
    if (pos.row <= size_.rows - 1 && pos.col <= size_.cols - 1) {
        ResetCache(pos);
        sheet_[pos.row][pos.col] = nullptr;
    }
    if (pos.row == printable_size_.rows - 1 || pos.col == printable_size_.cols - 1) {
        UpdatePrintableSize();
    }
}

Size Sheet::GetPrintableSize() const {
    return printable_size_;
}

void Sheet::PrintValue(std::ostream& output, const CellInterface::Value& value) const {
    if (std::holds_alternative<std::string>(value)) {
        output << std::get<std::string>(value);
    }
    else if (std::holds_alternative<double>(value)) {
        output << std::get<double>(value);
    }
    else {
        output << std::get<FormulaError>(value);
    }
}

void Sheet::PrintValues(std::ostream& output) const {
    for (int i = 0; i < printable_size_.rows; ++i) {
        for (int j = 0; j < printable_size_.cols - 1; ++j) {
            if (sheet_[i][j] != nullptr) {
                Position pos = { i, j };
                PrintValue(output, GetCell(pos)->GetValue());
                output << '\t';
            }
            else {
                output << '\t';
            }
        }

        if (sheet_[i][printable_size_.cols - 1] != nullptr) {
            Position pos = { i, printable_size_.cols - 1 };
            PrintValue(output, GetCell(pos)->GetValue());
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    for (int i = 0; i < printable_size_.rows; ++i) {
        for (int j = 0; j < printable_size_.cols - 1; ++j) {
            if (sheet_[i][j] != nullptr) {
                Position pos = { i, j };
                output << GetCell(pos)->GetText() << '\t';
            }
            else {
                output << '\t';
            }
        }

        if (sheet_[i][printable_size_.cols - 1] != nullptr) {
            Position pos = { i, printable_size_.cols - 1 };
            output << GetCell(pos)->GetText();
        }
        output << '\n';
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}