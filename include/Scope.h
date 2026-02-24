#ifndef SCOPE_H
#define SCOPE_H

#include "Token.h"
#include "SymbolTable.h"
#include <memory>

namespace ccomp {
class Variable;
class Function;
class FunctionParam;

class Scope {
public:
  enum StorageClass {
    STORAGE_STATIC = 0,
    STORAGE_EXTERN,
    STORAGE_AUTO,
  };

public:
  Scope();
  Scope(std::shared_ptr<Scope> enclosingScope);
  virtual ~Scope() = default;

  std::shared_ptr<Symbol> declare(const Token& name,
                                  const std::string& uniqueName,
                                  bool hasExternalLinkage,
                                  Function* fn, std::shared_ptr<Scope> scope);
  std::shared_ptr<Symbol> declare(const Token& name,
                                  const std::string& uniqueName,
                                  bool hasExternalLinkage,
                                  FunctionParam* param,
                                  std::shared_ptr<Scope> scope);
  std::shared_ptr<Symbol> declare(const Token& name,
                                  const std::string& uniqueName,
                                  bool hasExternalLinkage,
                                  Variable* var, std::shared_ptr<Scope> scope);

  std::shared_ptr<Scope> getEnclosingScope() const;
  int getNestingLevel() const;
  std::shared_ptr<Symbol> resolve(const Token& name) const;

private:
  int level_ = -1;
  std::shared_ptr<Scope> enclosingScope_;
  SymbolTable table_;
};
} // namespace ccomp

#endif // SCOPE_H
