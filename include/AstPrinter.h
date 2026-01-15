#ifndef AST_PRINTER_H
#define AST_PRINTER_H

#include "CObject.h"
#include "ast/Expr.h"
#include "ast/Stmt.h"
#include "Token.h"
#include <any>
#include <cstdio>
#include <memory>
#include <string>
#include <sys/cdefs.h>
#include <vector>

namespace ccomp {
class AstPrinter : public ExprVisitor, StmtVisitor {
public:
  std::shared_ptr<CObject> print(std::shared_ptr<Expr> expr) {
    std::any val = expr->accept(*this);
    return std::any_cast<std::shared_ptr<CObject>>(val);
  }
  std::shared_ptr<CObject> print(std::shared_ptr<Stmt> stmt) {
    std::any val = stmt->accept(*this);
    return std::any_cast<std::shared_ptr<CObject>>(val);
  }
  std::shared_ptr<CObject> print(std::vector<std::shared_ptr<Expr>> exprs) {
    for (auto expr : exprs) {
      expr->accept(*this);
      putchar(' ');
    }
    return nullptr;
  }
  std::shared_ptr<CObject> print(std::vector<std::shared_ptr<Stmt>> stmts) {
    for (auto stmt : stmts) {
      stmt->accept(*this);
      putchar('\n');
    }
    return nullptr;
  }
  std::any visitBlock(std::shared_ptr<Block> stmt) override {
    printf("(block\n");
    print(stmt->stmts);
    putchar(')');
    return nullptr;
    // return parenthesizeS("block", stmt->stmts);
  }
  std::any visitExpression(std::shared_ptr<Expression> stmt) override {
    return parenthesizeE("", {stmt->expr});
  }
  std::any visitFunction(std::shared_ptr<Function> stmt) override {
    std::string fn;
    fn.append("fn ");
    fn.append(stmt->name.lexeme);
    fn.append("(");
    for (auto p : stmt->params) {
      fn.append(p.lexeme);
      fn.append(" ");
    }
    fn.append(")");

    return parenthesizeS(fn, {stmt->body});
  }
  std::any visitIf(std::shared_ptr<If> stmt) override {
    printf("(if ");
    print(stmt->condition);
    return parenthesizeS("", {stmt->thenBranch, stmt->elseBranch});
  }
  std::any visitPrint(std::shared_ptr<Print> stmt) override {
    return parenthesizeE("print", {stmt->expr});
  }
  std::any visitReturn(std::shared_ptr<Return> Stmt) override {
    return parenthesizeE("return", {Stmt->value});
  }
  std::any visitWhile(std::shared_ptr<While> Stmt) override {
    printf("(while ");
    print(Stmt->condition);
    putchar('\n');
    print(Stmt->body);
    putchar(')');
    return nullptr;
  }
  std::any visitVar(std::shared_ptr<Var> Stmt) override {
    return parenthesizeE("var " + Stmt->name.lexeme, {Stmt->init});
  }

  std::any visitAssign(std::shared_ptr<Assign> expr) override {
    return parenthesizeE("= " + expr->name.lexeme, {expr->value});
  }
  std::any visitBinaryExpr(std::shared_ptr<BinaryExpr> expr) override {
    return parenthesizeE(expr->Operator.lexeme, {expr->left, expr->right});
  }
  std::any visitLogical(std::shared_ptr<Logical> expr) override {
    return parenthesizeE(expr->Operator.lexeme, {expr->left, expr->right});
  }
#if 0
  any visitSet(std::shared_ptr<Set> Expr __attribute_maybe_unused__) override {
    return nullptr;
  }
  virtual std::shared_ptr<CObject> visitThis(std::shared_ptr<This> Expr __attribute_maybe_unused__) override {
    return nullptr;
  }
  virtual std::shared_ptr<CObject>
  visitSuper(std::shared_ptr<Super> Expr __attribute_maybe_unused__) override {
    return nullptr;
  }
  std::shared_ptr<CObject> visitGroupingExpr(std::shared_ptr<GroupingExpr> expr) override {
    return parenthesizeE("group", {expr->expression});
  }
  std::shared_ptr<CObject> visitCall(std::shared_ptr<Call> Expr) override {
    // wrong
    printf("(call ");
    print(Expr->callee);
    print(Expr->args);
    putchar(')');
    return nullptr;
  }
  virtual std::shared_ptr<CObject> visitGet(std::shared_ptr<Get> Expr __attribute_maybe_unused__) override {
    return nullptr;
  }
#endif
  std::any visitLiteralExpr(std::shared_ptr<LiteralExpr> expr) override {
    printf(" %s", expr->value.c_str());
    return nullptr;
  }
  std::any visitUnaryExpr(std::shared_ptr<UnaryExpr> expr) override {
    return parenthesizeE(expr->Operator.lexeme, {expr->right});
  }
  std::any visitVariable(std::shared_ptr<Variable> expr) override {
    printf(" %s", expr->name.lexeme.c_str());
    return nullptr;
  }
  std::any parenthesizeS(std::string name, std::vector<std::shared_ptr<Stmt>> v) {
    std::string pp = "(" + name;
    // print
    printf("%s", pp.c_str());
    for (auto e : v) {
      if (e != nullptr)
        e->accept(*this);
    }
    putchar(')');
    return nullptr;
  }
  std::any parenthesizeE(std::string name, std::vector<std::shared_ptr<Expr>> v) {
    std::string pp = "(" + name;
    // print
    printf("%s", pp.c_str());
    for (auto e : v) {
      if (e != nullptr)
        e->accept(*this);
    }
    putchar(')');
    return nullptr;
  }
};
} // namespace ccomp

/// EXAMPLE USE:
// int main() {
//     std::unique_ptr<Expr> rootExpr(
//         new BinaryExpr(new UnaryExpr(*new Token(TokenType::MINUS, "-", "",
//         1),
//                                      new LiteralExpr("123")),
//                        *new Token(TokenType::STAR, "*", "", 1),
//                        new GroupingExpr(new LiteralExpr("45.67"))));
//     ASTPrinter pp;
//     pp.print(rootExpr.get());
//     std::cout << std::endl;
//     return 0;
// }

#endif // AST_PRINTER_H
