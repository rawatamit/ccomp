#include "AsmSymbolTable.h"

using namespace ccomp;

AsmSymbol::AsmSymbol(AsmInstType type, bool isStatic)
  : type_(type), isStatic_(isStatic), defined_(false), isFunction_(false)
{}

AsmSymbol::AsmSymbol(bool defined)
  : type_(ASM_TYPE_ERR), isStatic_(false), defined_(defined), isFunction_(true)
{}

bool AsmSymbol::isFunction() const {
  return isFunction_;
}

bool AsmSymbol::isObject() const {
  return !isFunction_;
}

bool AsmSymbol::isStatic() const {
  return isStatic_;
}

bool AsmSymbol::isDefined() const {
  return defined_;
}

AsmInstType AsmSymbol::getType() const {
  return type_;
}

const AsmSymbol* AsmSymTabT::addFunction(const std::string& name, bool defined) {
  auto sym = std::make_unique<AsmSymbol>(defined);
  auto it = symbols_.emplace(name, std::move(sym));
  return it.first->second.get();
}

const AsmSymbol* AsmSymTabT::addObject(const std::string& name,
                                       AsmInstType type, bool isStatic) {
  auto sym = std::make_unique<AsmSymbol>(type, isStatic);
  auto it = symbols_.emplace(name, std::move(sym));
  return it.first->second.get();
}

const AsmSymbol* AsmSymTabT::findSymbol(const std::string& name) const {
  auto it = symbols_.find(name);
  return (it == symbols_.end()) ? nullptr : it->second.get();
}
