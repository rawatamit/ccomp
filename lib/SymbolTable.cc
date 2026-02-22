#include "SymbolTable.h"
#include "Scope.h"
#include "Symbol.h"

using namespace ccomp;

SymbolTable::SymbolTable() :
  scope_(nullptr) {}

SymbolTable::SymbolTable(std::shared_ptr<Scope> scope) :
  scope_(scope) {}

std::shared_ptr<Symbol> SymbolTable::add(const Token &name,
                                         const std::string& uniqueName,
                                         bool hasExternalLinkage,
                                         const Function *fn,
                                         std::shared_ptr<Scope> scope) {
  auto it = identifiers_.emplace(
      name.lexeme, std::make_unique<FunctionSymbol>(
                       uniqueName, hasExternalLinkage, fn, scope));
  return it.first->second;
}

std::shared_ptr<Symbol> SymbolTable::add(const Token &name,
                                         const std::string& uniqueName,
                                         bool hasExternalLinkage,
                                         const FunctionParam *param,
                                         std::shared_ptr<Scope> scope) {
  auto it = identifiers_.emplace(
      name.lexeme, std::make_unique<FunctionParamSymbol>(
                       uniqueName, hasExternalLinkage, param, scope));
  return it.first->second;
}

std::shared_ptr<Symbol> SymbolTable::add(const Token &name,
                                         const std::string& uniqueName,
                                         bool hasExternalLinkage,
                                         const Variable *var,
                                         std::shared_ptr<Scope> scope) {
  auto it = identifiers_.emplace(
      name.lexeme, std::make_unique<VariableSymbol>(
                       uniqueName, hasExternalLinkage, var, scope));
  return it.first->second;
}

std::shared_ptr<Scope> SymbolTable::getScope() const
{ return scope_; }

std::shared_ptr<Symbol> SymbolTable::findSymbol(const Token& name) const {
  auto it = identifiers_.find(name.lexeme);
  if (it != identifiers_.end()) {
    return it->second;
  }
 
  return nullptr;
}
