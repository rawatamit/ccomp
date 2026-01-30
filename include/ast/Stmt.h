#ifndef Stmt_H_
#define Stmt_H_

#include "Token.h"
#include "Expr.h"
#include <memory>
#include <vector>
#include <variant>

namespace ccomp {
class Block;
class Expression;
class Function;
class If;
class Return;
class While;
class Decl;
class Null;
using Stmt = std::variant<Block, Expression, Function, If, Return, While, Decl, Null>;
class Block {
public: 
  Block(  std::vector<std::unique_ptr<Stmt>> stmts) :
    stmts(std::move(stmts)) {}
public: 
  std::vector<std::unique_ptr<Stmt>> stmts;
};

class Expression {
public: 
  Expression(  std::unique_ptr<Expr> expr) :
    expr(std::move(expr)) {}
public: 
  std::unique_ptr<Expr> expr;
};

class Function {
public: 
  Function(  Token name,   std::vector<Token> params,   std::vector<std::unique_ptr<Stmt>> body) :
    name(name), params(params), body(std::move(body)) {}
public: 
  Token name;
  std::vector<Token> params;
  std::vector<std::unique_ptr<Stmt>> body;
};

class If {
public: 
  If(  std::unique_ptr<Expr> condition,   std::unique_ptr<Stmt> thenBranch,   std::unique_ptr<Stmt> elseBranch) :
    condition(std::move(condition)), thenBranch(std::move(thenBranch)), elseBranch(std::move(elseBranch)) {}
public: 
  std::unique_ptr<Expr> condition;
  std::unique_ptr<Stmt> thenBranch;
  std::unique_ptr<Stmt> elseBranch;
};

class Return {
public: 
  Return(  Token keyword,   std::unique_ptr<Expr> value) :
    keyword(keyword), value(std::move(value)) {}
public: 
  Token keyword;
  std::unique_ptr<Expr> value;
};

class While {
public: 
  While(  std::unique_ptr<Expr> condition,   std::unique_ptr<Stmt> body) :
    condition(std::move(condition)), body(std::move(body)) {}
public: 
  std::unique_ptr<Expr> condition;
  std::unique_ptr<Stmt> body;
};

class Decl {
public: 
  Decl(  Token name,   std::unique_ptr<Expr> init) :
    name(name), init(std::move(init)) {}
public: 
  Token name;
  std::unique_ptr<Expr> init;
};

class Null {
public: 
  Null(  Token loc) :
    loc(loc) {}
public: 
  Token loc;
};

} // end namespace

#endif
