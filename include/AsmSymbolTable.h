#ifndef ASM_SYMBOL_TABLE_H
#define ASM_SYMBOL_TABLE_H

#include "ast/Asm.h"
#include <memory>
#include <unordered_map>

namespace ccomp {
class AsmSymbol {
public:
  AsmSymbol(AsmInstType type, bool isStatic);
  AsmSymbol(bool defined);
  ~AsmSymbol() = default;
  bool isFunction() const;
  bool isObject() const;
  bool isStatic() const;
  bool isDefined() const;
  AsmInstType getType() const;

private:
  AsmInstType type_;
  bool isStatic_;
  bool defined_;
  bool isFunction_;
};

class AsmSymTabT {
public:
  AsmSymTabT() = default;
  ~AsmSymTabT() = default;

  const AsmSymbol* addFunction(const std::string& name, bool defined);
  const AsmSymbol* addObject(const std::string& name,
                                       AsmInstType type, bool isStatic);
  const AsmSymbol* findSymbol(const std::string& name) const;

private:
  std::unordered_map<std::string, std::unique_ptr<AsmSymbol>> symbols_;
};
} // namespace ccomp

#endif // ASM_SYMBOL_TABLE_H
