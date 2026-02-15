#ifndef Stmt_H_
#define Stmt_H_

#include "Token.h"
#include "Expr.h"
#include "Scope.h"
#include "Type.h"
#include <memory>
#include <vector>
#include <variant>

namespace ccomp {
class Block;
class Expression;
class FunctionParam;
class Function;
class If;
class Return;
class DoWhile;
class While;
class For;
class Decl;
class Null;
class Break;
class Continue;
using Stmt = std::variant<Block, Expression, FunctionParam, Function, If, Return, DoWhile, While, For, Decl, Null, Break, Continue>;
class Block {
public: 
  Block(std::vector<std::unique_ptr<Stmt>> stmts) :
    stmts(std::move(stmts)) {}
public: 
  std::vector<std::unique_ptr<Stmt>> stmts;
};

class Expression {
public: 
  Expression(std::unique_ptr<Expr> expr) :
    expr(std::move(expr)) {}
public: 
  std::unique_ptr<Expr> expr;
};

class FunctionParam {
public: 
  FunctionParam(Token type, Token name) :
    type(type), name(name) {}
public: 
  Token type;
  Token name;
  int level;
  const Type* evalty;
};

class Function {
public: 
  Function(Token name, std::vector<std::unique_ptr<Stmt>> params, std::unique_ptr<Stmt> body) :
    name(name), params(std::move(params)), body(std::move(body)) {}
public: 
  Token name;
  std::vector<std::unique_ptr<Stmt>> params;
  std::unique_ptr<Stmt> body;
  std::shared_ptr<Scope> scope;
  std::unique_ptr<FunctionType> evalty;
};

class If {
public: 
  If(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> thenBranch, std::unique_ptr<Stmt> elseBranch) :
    condition(std::move(condition)), thenBranch(std::move(thenBranch)), elseBranch(std::move(elseBranch)) {}
public: 
  std::unique_ptr<Expr> condition;
  std::unique_ptr<Stmt> thenBranch;
  std::unique_ptr<Stmt> elseBranch;
};

class Return {
public: 
  Return(Token keyword, std::unique_ptr<Expr> value) :
    keyword(keyword), value(std::move(value)) {}
public: 
  Token keyword;
  std::unique_ptr<Expr> value;
};

class DoWhile {
public: 
  DoWhile(std::unique_ptr<Stmt> body, std::unique_ptr<Expr> condition) :
    body(std::move(body)), condition(std::move(condition)) {}
public: 
  std::unique_ptr<Stmt> body;
  std::unique_ptr<Expr> condition;
  int loop_label;
};

class While {
public: 
  While(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body) :
    condition(std::move(condition)), body(std::move(body)) {}
public: 
  std::unique_ptr<Expr> condition;
  std::unique_ptr<Stmt> body;
  int loop_label;
};

class For {
public: 
  For(std::unique_ptr<Stmt> init, std::unique_ptr<Expr> condition, std::unique_ptr<Expr> post, std::unique_ptr<Stmt> body) :
    init(std::move(init)), condition(std::move(condition)), post(std::move(post)), body(std::move(body)) {}
public: 
  std::unique_ptr<Stmt> init;
  std::unique_ptr<Expr> condition;
  std::unique_ptr<Expr> post;
  std::unique_ptr<Stmt> body;
  int loop_label;
};

class Decl {
public: 
  Decl(std::unique_ptr<Expr> name, std::unique_ptr<Expr> init) :
    name(std::move(name)), init(std::move(init)) {}
public: 
  std::unique_ptr<Expr> name;
  std::unique_ptr<Expr> init;
};

class Null {
public: 
  Null(Token loc) :
    loc(loc) {}
public: 
  Token loc;
};

class Break {
public: 
  Break(Token loc) :
    loc(loc) {}
public: 
  Token loc;
  int loop_label;
};

class Continue {
public: 
  Continue(Token loc) :
    loc(loc) {}
public: 
  Token loc;
  int loop_label;
};

} // end namespace

#endif
