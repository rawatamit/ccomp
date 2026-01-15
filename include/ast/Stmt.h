#ifndef Stmt_H_
#define Stmt_H_

#include "Token.h"
#include <memory>
#include <any>
#include <vector>
namespace ccomp {
class Stmt;
class Block;
class Expression;
class Function;
class If;
class Print;
class Return;
class While;
class Var;
class Expr;

class StmtVisitor {
public:
  virtual ~StmtVisitor() {}
  virtual std::any     visitBlock      (std::shared_ptr<Block      > Stmt) = 0;
  virtual std::any     visitExpression (std::shared_ptr<Expression > Stmt) = 0;
  virtual std::any     visitFunction   (std::shared_ptr<Function   > Stmt) = 0;
  virtual std::any     visitIf         (std::shared_ptr<If         > Stmt) = 0;
  virtual std::any     visitPrint      (std::shared_ptr<Print      > Stmt) = 0;
  virtual std::any     visitReturn     (std::shared_ptr<Return     > Stmt) = 0;
  virtual std::any     visitWhile      (std::shared_ptr<While      > Stmt) = 0;
  virtual std::any     visitVar        (std::shared_ptr<Var        > Stmt) = 0;
};

class Stmt {
public:
  virtual ~Stmt() {}
  virtual std::any accept(StmtVisitor& visitor) = 0;
};

class Block       : public std::enable_shared_from_this<Block      >, public Stmt { 
public: 
  Block      (   std::vector<std::shared_ptr<Stmt>> stmts)  :
    stmts(stmts) {}
 std::any accept(StmtVisitor& visitor) override {
    std::shared_ptr<Block      > p{shared_from_this()};
    return visitor.visitBlock      (p);
  }
public: 
   std::vector<std::shared_ptr<Stmt>> stmts;
};

class Expression  : public std::enable_shared_from_this<Expression >, public Stmt { 
public: 
  Expression (   std::shared_ptr<Expr> expr)  :
    expr(expr) {}
 std::any accept(StmtVisitor& visitor) override {
    std::shared_ptr<Expression > p{shared_from_this()};
    return visitor.visitExpression (p);
  }
public: 
   std::shared_ptr<Expr> expr;
};

class Function    : public std::enable_shared_from_this<Function   >, public Stmt { 
public: 
  Function   (   Token name,    std::vector<Token> params,    std::vector<std::shared_ptr<Stmt>> body)  :
    name(name), params(params), body(body) {}
 std::any accept(StmtVisitor& visitor) override {
    std::shared_ptr<Function   > p{shared_from_this()};
    return visitor.visitFunction   (p);
  }
public: 
   Token name;
   std::vector<Token> params;
   std::vector<std::shared_ptr<Stmt>> body;
};

class If          : public std::enable_shared_from_this<If         >, public Stmt { 
public: 
  If         (   std::shared_ptr<Expr> condition,    std::shared_ptr<Stmt> thenBranch,    std::shared_ptr<Stmt> elseBranch)  :
    condition(condition), thenBranch(thenBranch), elseBranch(elseBranch) {}
 std::any accept(StmtVisitor& visitor) override {
    std::shared_ptr<If         > p{shared_from_this()};
    return visitor.visitIf         (p);
  }
public: 
   std::shared_ptr<Expr> condition;
   std::shared_ptr<Stmt> thenBranch;
   std::shared_ptr<Stmt> elseBranch;
};

class Print       : public std::enable_shared_from_this<Print      >, public Stmt { 
public: 
  Print      (   std::shared_ptr<Expr> expr)  :
    expr(expr) {}
 std::any accept(StmtVisitor& visitor) override {
    std::shared_ptr<Print      > p{shared_from_this()};
    return visitor.visitPrint      (p);
  }
public: 
   std::shared_ptr<Expr> expr;
};

class Return      : public std::enable_shared_from_this<Return     >, public Stmt { 
public: 
  Return     (   Token keyword,    std::shared_ptr<Expr> value)  :
    keyword(keyword), value(value) {}
 std::any accept(StmtVisitor& visitor) override {
    std::shared_ptr<Return     > p{shared_from_this()};
    return visitor.visitReturn     (p);
  }
public: 
   Token keyword;
   std::shared_ptr<Expr> value;
};

class While       : public std::enable_shared_from_this<While      >, public Stmt { 
public: 
  While      (   std::shared_ptr<Expr> condition,    std::shared_ptr<Stmt> body)  :
    condition(condition), body(body) {}
 std::any accept(StmtVisitor& visitor) override {
    std::shared_ptr<While      > p{shared_from_this()};
    return visitor.visitWhile      (p);
  }
public: 
   std::shared_ptr<Expr> condition;
   std::shared_ptr<Stmt> body;
};

class Var         : public std::enable_shared_from_this<Var        >, public Stmt { 
public: 
  Var        (   Token name,    std::shared_ptr<Expr> init)  :
    name(name), init(init) {}
 std::any accept(StmtVisitor& visitor) override {
    std::shared_ptr<Var        > p{shared_from_this()};
    return visitor.visitVar        (p);
  }
public: 
   Token name;
   std::shared_ptr<Expr> init;
};

} // end namespace

#endif
