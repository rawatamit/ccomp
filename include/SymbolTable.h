#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "Symbol.h"
#include <memory>
#include <unordered_map>

namespace ccomp {
class Token;
class Scope;
class Variable;
class Function;
class FunctionParam;

class SymbolTable {
public:
  SymbolTable();
  SymbolTable(std::shared_ptr<Scope> scope);

  std::shared_ptr<Symbol> add(const Token& name, const std::string& uniqueName,
                              bool hasExternalLinkage, const Function* fn,
                              std::shared_ptr<Scope> scope);
  std::shared_ptr<Symbol> add(const Token& name, const std::string& uniqueName,
                              bool hasExternalLinkage,
                              const FunctionParam* param,
                              std::shared_ptr<Scope> scope);
  std::shared_ptr<Symbol> add(const Token& name, const std::string& uniqueName,
                              bool hasExternalLinkage, const Variable* var,
                              std::shared_ptr<Scope> scope);

  std::shared_ptr<Scope> getScope() const;
  std::shared_ptr<Symbol> findSymbol(const Token& name) const;

private:
  std::shared_ptr<Scope> scope_;
  bool hasLinkage_;
  std::unordered_map<std::string, std::shared_ptr<Symbol>> identifiers_;
};
} // namespace ccomp

#endif // SYMBOL_TABLE_H
