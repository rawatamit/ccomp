#ifndef Expr_H_
#define Expr_H_

#include "Token.h"
#include "Scope.h"
#include "Type.h"
#include <memory>
#include <string>
#include <vector>
#include <variant>

namespace ccomp {
class Assign;
class Conditional;
class BinaryExpr;
class LiteralExpr;
class UnaryExpr;
class Variable;
class Call;
using Expr = std::variant<Assign, Conditional, BinaryExpr, LiteralExpr, UnaryExpr, Variable, Call>;
class Assign {
public: 
  Assign(std::unique_ptr<Expr> lvalue, std::unique_ptr<Expr> value) :
    lvalue(std::move(lvalue)), value(std::move(value)) {}
public: 
  std::unique_ptr<Expr> lvalue;
  std::unique_ptr<Expr> value;
};

class Conditional {
public: 
  Conditional(std::unique_ptr<Expr> condition, std::unique_ptr<Expr> thenExp, std::unique_ptr<Expr> elseExp) :
    condition(std::move(condition)), thenExp(std::move(thenExp)), elseExp(std::move(elseExp)) {}
public: 
  std::unique_ptr<Expr> condition;
  std::unique_ptr<Expr> thenExp;
  std::unique_ptr<Expr> elseExp;
};

class BinaryExpr {
public: 
  BinaryExpr(std::unique_ptr<Expr> left, Token Operator, std::unique_ptr<Expr> right) :
    left(std::move(left)), Operator(Operator), right(std::move(right)) {}
public: 
  std::unique_ptr<Expr> left;
  Token Operator;
  std::unique_ptr<Expr> right;
};

class LiteralExpr {
public: 
  LiteralExpr(TokenType type, std::string value) :
    type(type), value(value) {}
public: 
  TokenType type;
  std::string value;
};

class UnaryExpr {
public: 
  UnaryExpr(Token Operator, std::unique_ptr<Expr> right) :
    Operator(Operator), right(std::move(right)) {}
public: 
  Token Operator;
  std::unique_ptr<Expr> right;
};

class Variable {
public: 
  Variable(Token name) :
    name(name) {}
public: 
  Token name;
  std::shared_ptr<Symbol> sym;
  const Type* evalty=0;
};

class Call {
public: 
  Call(std::unique_ptr<Expr> callee, std::vector<std::unique_ptr<Expr>> args) :
    callee(std::move(callee)), args(std::move(args)) {}
public: 
  std::unique_ptr<Expr> callee;
  std::vector<std::unique_ptr<Expr>> args;
  const Function* fn=0;
};

} // end namespace

#endif
