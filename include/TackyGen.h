#ifndef TACKYGEN_H
#define TACKYGEN_H

#include "ErrorHandler.h"
#include "ast/Tacky.h"
#include "ast/Expr.h"
#include "ast/Stmt.h"

namespace ccomp {
class TackyGen : public ExprVisitor, StmtVisitor {
public:
  TackyGen(const std::vector<std::shared_ptr<Stmt>>& stmts, ErrorHandler& errorHandler);
  std::shared_ptr<Tacky> gen();

private:
  const std::vector<std::shared_ptr<Stmt>>& stmts_;
  ErrorHandler& errorHandler_;

  std::vector<std::shared_ptr<Tacky>> gen(std::shared_ptr<Expr> expr);
  std::vector<std::shared_ptr<Tacky>> gen(std::shared_ptr<Stmt> stmt);
  std::vector<std::shared_ptr<Tacky>> gen(std::vector<std::shared_ptr<Expr>> exprs);
  std::vector<std::shared_ptr<Tacky>> gen(std::vector<std::shared_ptr<Stmt>> stmts);

  std::any genLogical(std::shared_ptr<BinaryExpr> expr);
  std::string unique_var();
  std::shared_ptr<TackyLabel> unique_label(const std::string& desc);

private:
  std::any visitBlock(std::shared_ptr<Block> stmt) override;
  std::any visitExpression(std::shared_ptr<Expression> stmt) override;
  std::any visitFunction(std::shared_ptr<Function> stmt) override;
  std::any visitIf(std::shared_ptr<If> stmt) override;
  std::any visitPrint(std::shared_ptr<Print> stmt) override;
  std::any visitReturn(std::shared_ptr<Return> Stmt) override;
  std::any visitWhile(std::shared_ptr<While> Stmt) override;
  std::any visitDecl(std::shared_ptr<Decl> Stmt) override;
  std::any visitNull(std::shared_ptr<Null> Stmt) override;
  std::any visitAssign(std::shared_ptr<Assign> expr) override;
  std::any visitBinaryExpr(std::shared_ptr<BinaryExpr> expr) override;
  std::any visitLiteralExpr(std::shared_ptr<LiteralExpr> expr) override;
  std::any visitUnaryExpr(std::shared_ptr<UnaryExpr> expr) override;
  std::any visitVariable(std::shared_ptr<Variable> expr) override;
};
}

#endif // TACKYGEN_H
