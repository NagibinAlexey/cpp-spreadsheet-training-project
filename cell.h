#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <unordered_set>
#include <optional>

class Cell : public CellInterface {
public:
    explicit Cell(SheetInterface& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    const SheetInterface& GetSheet() const;
    void AddDependencedCell(Position pos);
    void RemoveDependencedCell(Position pos);
    std::vector<Position> GetReferencedCells() const override;
    std::unordered_set<Position, PositionHash> GetDependencedCells() const;
    void ResetCacheToNull();
    bool IsCached() { return impl_->IsCached(); }

    bool IsReferenced() const;

private: 
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

    class Impl {
    public:
        virtual ~Impl() {}
        virtual void Set(std::string text) = 0;
        virtual CellInterface::Value GetValue() = 0;
        virtual std::vector<Position> GetReferencedCells() const = 0;
        virtual std::string GetText() const = 0;
        virtual void ResetCacheToNull() = 0;
        virtual bool IsCached() = 0;
    };

    class EmptyImpl : public Impl {
    public:
        void Set(std::string text) override {};
        CellInterface::Value GetValue() override;
        std::vector<Position> GetReferencedCells() const override;
        std::string GetText() const override;
        void ResetCacheToNull() override {};
        bool IsCached() override { return false; }
    };

    class TextImpl : public Impl {
    public:
        void Set(std::string text) override;
        CellInterface::Value GetValue() override;
        std::vector<Position> GetReferencedCells() const override;
        std::string GetText() const override;
        void ResetCacheToNull() override {};
        bool IsCached() override { return false; }
    private:
        std::string text_;
    };

    class FormulaImpl : public Impl {
    public:
        FormulaImpl(const SheetInterface& sheet);
        void Set(std::string text) override;
        CellInterface::Value GetValue() override;
        std::vector<Position> GetReferencedCells() const override;
        std::string GetText() const override;
        void ResetCacheToNull() override;
        bool IsCached() override { return cache_ != std::nullopt; }
    private:
        std::string text_;
        const SheetInterface& sheet_;
        std::vector<Position> referenced_cells_;
        std::optional<CellInterface::Value> cache_;
    };

    SheetInterface& sheet_;
    std::unique_ptr<Impl> impl_ = nullptr;
    std::unordered_set<Position, PositionHash> dependenced_cells_;
};