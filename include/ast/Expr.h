#ifndef Expr_H_
#define Expr_H_

#include "Token.h"
#include "Scope.h"
#include <memory>
#include <string>
#include <vector>
#include <variant>

namespace ccomp {
class Type;
class Assign;
class Conditional;
class BinaryExpr;
class Int32Exp;
class Int64Exp;
class StringExp;
class CastExpr;
class UnaryExpr;
class Variable;
class Call;
typedef const Type* type_ptr;
using Expr = std::variant<Assign, Conditional, BinaryExpr, Int32Exp, Int64Exp, StringExp, CastExpr, UnaryExpr, Variable, Call>;
class Assign {
public: 
  Assign(std::unique_ptr<Expr> lvalue, std::unique_ptr<Expr> value) :
    lvalue(std::move(lvalue)), value(std::move(value)) {}
public: 
  std::unique_ptr<Expr> lvalue;
  std::unique_ptr<Expr> value;
  type_ptr evalty=0;
};

class Conditional {
public: 
  Conditional(std::unique_ptr<Expr> condition, std::unique_ptr<Expr> thenExp, std::unique_ptr<Expr> elseExp) :
    condition(std::move(condition)), thenExp(std::move(thenExp)), elseExp(std::move(elseExp)) {}
public: 
  std::unique_ptr<Expr> condition;
  std::unique_ptr<Expr> thenExp;
  std::unique_ptr<Expr> elseExp;
  type_ptr evalty=0;
};

class BinaryExpr {
public: 
  BinaryExpr(std::unique_ptr<Expr> left, Token op, std::unique_ptr<Expr> right) :
    left(std::move(left)), op(op), right(std::move(right)) {}
public: 
  std::unique_ptr<Expr> left;
  Token op;
  std::unique_ptr<Expr> right;
  type_ptr evalty=0;
};

class Int32Exp {
public: 
  Int32Exp(Token tok, int int32) :
    tok(tok), int32(int32) {}
public: 
  Token tok;
  int int32;
  type_ptr evalty=0;
};

class Int64Exp {
public: 
  Int64Exp(Token tok, long int64) :
    tok(tok), int64(int64) {}
public: 
  Token tok;
  long int64;
  type_ptr evalty=0;
};

class StringExp {
public: 
  StringExp(Token tok, std::string str) :
    tok(tok), str(str) {}
public: 
  Token tok;
  std::string str;
  type_ptr evalty=0;
};

class CastExpr {
public: 
  CastExpr(Token type, std::unique_ptr<Expr> expr) :
    type(type), expr(std::move(expr)) {}
public: 
  Token type;
  std::unique_ptr<Expr> expr;
  type_ptr evalty=0;
  type_ptr exprty=0;
};

class UnaryExpr {
public: 
  UnaryExpr(Token op, std::unique_ptr<Expr> right) :
    op(op), right(std::move(right)) {}
public: 
  Token op;
  std::unique_ptr<Expr> right;
  type_ptr evalty=0;
};

class Variable {
public: 
  Variable(Token name) :
    name(name) {}
public: 
  Token name;
  std::shared_ptr<Symbol> sym;
  type_ptr evalty=0;
};

class Call {
public: 
  Call(std::unique_ptr<Expr> callee, std::vector<std::unique_ptr<Expr>> args) :
    callee(std::move(callee)), args(std::move(args)) {}
public: 
  std::unique_ptr<Expr> callee;
  std::vector<std::unique_ptr<Expr>> args;
  const Function* fn=0;
  type_ptr evalty=0;
};

} // end namespace

#endif
