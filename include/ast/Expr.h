#ifndef Expr_H_
#define Expr_H_

#include "Token.h"
#include <memory>
#include <any>
#include <vector>
namespace ccomp {
class Expr;
class Assign;
class BinaryExpr;
class LiteralExpr;
class Logical;
class UnaryExpr;
class Variable;
class Expr;

class ExprVisitor {
public:
  virtual ~ExprVisitor() {}
  virtual std::any     visitAssign       (std::shared_ptr<Assign       > Expr __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitBinaryExpr   (std::shared_ptr<BinaryExpr   > Expr __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitLiteralExpr (std::shared_ptr<LiteralExpr > Expr __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitLogical  (std::shared_ptr<Logical  > Expr __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitUnaryExpr    (std::shared_ptr<UnaryExpr    > Expr __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitVariable     (std::shared_ptr<Variable     > Expr __attribute_maybe_unused__) { return nullptr; }
};

class Expr {
public:
  virtual ~Expr() {}
  virtual std::any accept(ExprVisitor& visitor) = 0;
};

class Assign        : public std::enable_shared_from_this<Assign       >, public Expr { 
public: 
  Assign       (  Token name,    std::shared_ptr<Expr> value)  :
    name(name), value(value) {}
 std::any accept(ExprVisitor& visitor) override {
    std::shared_ptr<Assign       > p{shared_from_this()};
    return visitor.visitAssign       (p);
  }
public: 
  Token name;
   std::shared_ptr<Expr> value;
};

class BinaryExpr    : public std::enable_shared_from_this<BinaryExpr   >, public Expr { 
public: 
  BinaryExpr   (  std::shared_ptr<Expr> left,    Token Operator,    std::shared_ptr<Expr> right)  :
    left(left), Operator(Operator), right(right) {}
 std::any accept(ExprVisitor& visitor) override {
    std::shared_ptr<BinaryExpr   > p{shared_from_this()};
    return visitor.visitBinaryExpr   (p);
  }
public: 
  std::shared_ptr<Expr> left;
   Token Operator;
   std::shared_ptr<Expr> right;
};

class LiteralExpr  : public std::enable_shared_from_this<LiteralExpr >, public Expr { 
public: 
  LiteralExpr (   TokenType type,    std::string value)  :
    type(type), value(value) {}
 std::any accept(ExprVisitor& visitor) override {
    std::shared_ptr<LiteralExpr > p{shared_from_this()};
    return visitor.visitLiteralExpr (p);
  }
public: 
   TokenType type;
   std::string value;
};

class Logical   : public std::enable_shared_from_this<Logical  >, public Expr { 
public: 
  Logical  (   std::shared_ptr<Expr> left,    Token Operator,    std::shared_ptr<Expr> right)  :
    left(left), Operator(Operator), right(right) {}
 std::any accept(ExprVisitor& visitor) override {
    std::shared_ptr<Logical  > p{shared_from_this()};
    return visitor.visitLogical  (p);
  }
public: 
   std::shared_ptr<Expr> left;
   Token Operator;
   std::shared_ptr<Expr> right;
};

class UnaryExpr     : public std::enable_shared_from_this<UnaryExpr    >, public Expr { 
public: 
  UnaryExpr    (  Token Operator,    std::shared_ptr<Expr> right)  :
    Operator(Operator), right(right) {}
 std::any accept(ExprVisitor& visitor) override {
    std::shared_ptr<UnaryExpr    > p{shared_from_this()};
    return visitor.visitUnaryExpr    (p);
  }
public: 
  Token Operator;
   std::shared_ptr<Expr> right;
};

class Variable      : public std::enable_shared_from_this<Variable     >, public Expr { 
public: 
  Variable     (  Token name)  :
    name(name) {}
 std::any accept(ExprVisitor& visitor) override {
    std::shared_ptr<Variable     > p{shared_from_this()};
    return visitor.visitVariable     (p);
  }
public: 
  Token name;
};

} // end namespace

#endif
