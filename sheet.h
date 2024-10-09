#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <algorithm>

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;
    void ClearDependences(Position pos);

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

    const Cell* GetConcreteCell(Position pos) const;
    Cell* GetConcreteCell(Position pos);

private:
    std::vector<std::vector<std::unique_ptr<Cell>>> sheet_;
    Size printable_size_ = { 0,0 };
    Size size_ = { 0,0 };

    void ResizeToPosition(Position pos);
    void PrintValue(std::ostream& output, const CellInterface::Value& value) const;
    void UpdatePrintableSize();
    void ResetCache(Position start_pos);
    void SetDependences(const Position pos);
    void CreateNewCell(const Position pos, std::string text);
    void EditCell(const Position pos, std::string text);
    bool HasCircularDependency(const Position pos) const;
    void HasCircularDependencyImpl(const Position pos, const Position init_pos, bool& has_circular) const;
};