#ifndef SCOPE_H
#define SCOPE_H

#include "Token.h"
#include <any>
#include <memory>
#include <string>
#include <unordered_map>

namespace ccomp {
class Variable;
class Function;
class FunctionParam;

class Scope {
public:
  enum Linkage {
    LINKAGE_INTERNAL = 0,
    LINKAGE_EXTERNAL,
    LINKAGE_ERROR,
  };

  struct ScopeDetail {
    std::shared_ptr<Scope> scope;
    std::any value;
    Linkage linkage;
  };

protected:
  Scope(std::shared_ptr<Scope> enclosingScope) :
    level_(enclosingScope ? (enclosingScope->getLevel() + 1) : 0),
    enclosingScope_(enclosingScope) {}

public:
  virtual ~Scope() {}

  void declare(const Token& name, std::any obj, Linkage linkage) {
    identifiers_[name.lexeme] =
      ScopeDetail(std::make_shared<Scope>(*this), obj, linkage);
  }

  std::shared_ptr<Scope> getEnclosingScope() const {
    return enclosingScope_;
  }

  int getLevel() const {
    return level_;
  }

  ScopeDetail resolve(const Token& name) const {
    auto it = identifiers_.find(name.lexeme);
    // it = [scope, <id, linkage>]
    if (it != identifiers_.end()) {
      // scope, id
      return it->second;
    }

    if (auto scope = getEnclosingScope()) {
      return scope->resolve(name);
    }

    return {nullptr, false, LINKAGE_ERROR};
  }

private:
  int level_ = -1;
  std::shared_ptr<Scope> enclosingScope_;
  std::unordered_map<std::string, ScopeDetail> identifiers_;
};

struct GlobalScope : public Scope {
  GlobalScope() :
    Scope(nullptr) {}
};

struct FunctionScope : public Scope {
  FunctionScope(std::shared_ptr<Scope> scope) :
    Scope(scope)
  {}
};

struct LocalScope : public Scope {
  LocalScope(std::shared_ptr<Scope> scope) :
    Scope(scope)
  {}
};
} // namespace ccomp

#endif // SCOPE_H
