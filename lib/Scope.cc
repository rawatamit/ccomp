#include "Scope.h"

using namespace ccomp;

Scope::Scope() :
  Scope(nullptr)
{}

Scope::Scope(std::shared_ptr<Scope> enclosingScope) :
  level_(enclosingScope ? (enclosingScope->getLevel() + 1) : 0),
  enclosingScope_(enclosingScope)
{}

std::shared_ptr<Symbol> Scope::declare(const Token& name,
                                       const std::string& uniqueName,
                                       bool hasExternalLinkage,
                                       Function* fn,
                                       std::shared_ptr<Scope> scope) {
  return table_.add(name, uniqueName, hasExternalLinkage, fn, scope);
}

std::shared_ptr<Symbol> Scope::declare(const Token& name,
                                       const std::string& uniqueName,
                                       bool hasExternalLinkage,
                                       FunctionParam* param,
                                       std::shared_ptr<Scope> scope) {
  return table_.add(name, uniqueName, hasExternalLinkage, param, scope);
}

std::shared_ptr<Symbol> Scope::declare(const Token& name,
                                       const std::string& uniqueName,
                                       bool hasExternalLinkage,
                                       Variable* var,
                                       std::shared_ptr<Scope> scope) {
  return table_.add(name, uniqueName, hasExternalLinkage, var, scope);
}

std::shared_ptr<Scope> Scope::getEnclosingScope() const {
  return enclosingScope_;
}

int Scope::getLevel() const {
  return level_;
}

std::shared_ptr<Symbol> Scope::resolve(const Token& name) const {
  if (auto sym = table_.findSymbol(name)) {
    return sym;
  }

  if (auto scope = getEnclosingScope()) {
    return scope->resolve(name);
  }

  return nullptr;
}
