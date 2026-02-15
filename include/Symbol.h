#ifndef SYMBOL_H
#define SYMBOL_H

#include "Token.h"
#include "Type.h"

namespace ccomp {
struct Symbol {
protected:
  Symbol(const Token &name, const Type &type) : name(name), type(type) {}

public:
  Token name;
  const Type &type;
};

struct FunctionSymbol : public Symbol {
  FunctionSymbol(const Token &name, const Type &type) : Symbol(name, type) {}
};

struct StructSymbol : public Symbol {
  StructSymbol(const Token &name, const Type &type) : Symbol(name, type) {}
};
} // namespace ccomp

#endif // SYMBOL_H
