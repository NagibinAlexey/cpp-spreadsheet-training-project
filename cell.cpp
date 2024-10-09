#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

Cell::Cell(SheetInterface& sheet) : sheet_(sheet) {
	impl_ = std::make_unique<EmptyImpl>();
}

Cell::~Cell() {}

void Cell::Set(std::string text) {
	if (text[0] == FORMULA_SIGN && text.size() != 1) {
		impl_ = std::make_unique<FormulaImpl>(sheet_);
		impl_->Set(text);
	}
	else {
		impl_ = std::make_unique<TextImpl>();
		impl_->Set(text);
	}
}

void Cell::Clear() {
	impl_ = std::make_unique<EmptyImpl>();
}

void Cell::ResetCacheToNull() {
	impl_->ResetCacheToNull();
}

Cell::Value Cell::GetValue() const {
	return impl_->GetValue();
}

std::string Cell::GetText() const {
	return impl_->GetText();
}

const SheetInterface& Cell::GetSheet() const {
	return sheet_;
}

void Cell::AddDependencedCell(Position pos) {
	dependenced_cells_.insert(pos);
}

void Cell::RemoveDependencedCell(Position pos) {
	dependenced_cells_.erase(pos);
}

std::vector<Position> Cell::GetReferencedCells() const {
	return impl_->GetReferencedCells();
}

std::unordered_set<Position, PositionHash> Cell::GetDependencedCells() const {
	return dependenced_cells_;
}

bool Cell::IsReferenced() const {
	return !dependenced_cells_.empty();
}

void Cell::TextImpl::Set(std::string text)
{
	text_ = std::move(text);
}

CellInterface::Value Cell::TextImpl::GetValue() 
{
	if (text_.empty()) return 0.0;

	if (text_[0] == ESCAPE_SIGN) {
		return std::string{text_.begin() + 1, text_.end()};
	}
	return text_;
}

std::string Cell::TextImpl::GetText() const
{
	return text_;
}

std::vector<Position> Cell::TextImpl::GetReferencedCells() const {
	return {};
}

Cell::FormulaImpl::FormulaImpl(const SheetInterface& sheet) : sheet_(sheet) {}

void Cell::FormulaImpl::Set(std::string text)
{
	auto formula = ParseFormula(std::string{text.begin() + 1, text.end()});
	referenced_cells_ = formula->GetReferencedCells();

	text_ = std::move(text);
}

CellInterface::Value Cell::FormulaImpl::GetValue() 
{
	if (cache_ == std::nullopt) {

		CellInterface::Value value_;
		auto formula = ParseFormula(std::string{text_.begin() + 1, text_.end()});
		FormulaInterface::Value result_ = formula->Evaluate(sheet_);

		if (std::holds_alternative<double>(result_)) {
			value_ = std::get<double>(result_);
		}

		else if (std::holds_alternative<FormulaError>(result_)) {
			value_ = std::get<FormulaError>(result_);
		}

		cache_ = value_;
	}

	return cache_.value();
}

void Cell::FormulaImpl::ResetCacheToNull() {
	cache_ = std::nullopt;
}

std::string Cell::FormulaImpl::GetText() const
{
	std::string result = "=";
	auto formula = ParseFormula(std::string{text_.begin() + 1, text_.end()});
	result += formula->GetExpression();
	return result;
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
	return referenced_cells_;
}

CellInterface::Value Cell::EmptyImpl::GetValue() 
{
	return 0.0;
}

std::string Cell::EmptyImpl::GetText() const
{
	return {};
}

std::vector<Position> Cell::EmptyImpl::GetReferencedCells() const{
	return {};
}
