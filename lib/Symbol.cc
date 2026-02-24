#include "Symbol.h"
#include "Scope.h"

using namespace ccomp;

Symbol::Symbol(const std::string& name, bool hasExternalLinkage,
               std::shared_ptr<Scope> scope) :
  name_(name), hasExternalLinkage_(hasExternalLinkage), scope_(scope)
{}

const std::string& Symbol::getName() const
{ return name_; }

bool Symbol::hasExternalLinkage() const
{ return hasExternalLinkage_; }

int Symbol::getNestingLevel() const {
  if (scope_) {
    return scope_->getNestingLevel();
  }

  return -1;
}

std::shared_ptr<Scope> Symbol::getScope() const {
  return scope_;
}

bool Symbol::isFunction() const
{ return false; }

const Function* Symbol::getFunction() const
{ return nullptr; }

bool Symbol::isFunctionParam() const
{ return false; }

const FunctionParam* Symbol::getFunctionParam() const
{ return nullptr; }

bool Symbol::isVariable() const
{ return false; }

const Variable* Symbol::getVariable() const
{ return nullptr; }

void Symbol::setType(const Type* ty)
{ ty_ = ty; }

const Type* Symbol::getType() const
{ return ty_; }

void Symbol::setAttrs(std::unique_ptr<SymbolAttrs> attrs) {
  attrs_ = std::move(attrs);
}

const SymbolAttrs* Symbol::getAttrs() const
{ return attrs_.get(); }

FunctionSymbol::FunctionSymbol(const std::string& name, bool hasExternalLinkage,
                               const Function* fn,
                               std::shared_ptr<Scope> scope) :
  Symbol(name, hasExternalLinkage, scope), fn_(fn) {}

bool FunctionSymbol::isFunction() const
{ return true; }

const Function* FunctionSymbol::getFunction() const
{ return fn_; }

FunctionParamSymbol::FunctionParamSymbol(
  const std::string& name, bool hasExternalLinkage,
  const FunctionParam* param, std::shared_ptr<Scope> scope) :
  Symbol(name, hasExternalLinkage, scope), param_(param) {}

bool FunctionParamSymbol::isFunctionParam() const
{ return true; }

const FunctionParam* FunctionParamSymbol::getFunctionParam() const
{ return param_; }

VariableSymbol::VariableSymbol(
  const std::string& name, bool hasExternalLinkage,
  const Variable* var,
  std::shared_ptr<Scope> scope) :
  Symbol(name, hasExternalLinkage, scope), var_(var)
{}

bool VariableSymbol::isVariable() const
{ return true; }

const Variable* VariableSymbol::getVariable() const
{ return var_; }
