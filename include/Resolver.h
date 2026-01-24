#ifndef _RESOLVER_H_
#define _RESOLVER_H_

#include "ErrorHandler.h"
#include "Token.h"
#include "ast/Expr.h"
#include "ast/Stmt.h"

#include <map>
#include <vector>

namespace ccomp {

class Resolver : public ExprVisitor, public StmtVisitor {
private:
  enum FunctionType {
    NONEF,
    FUNCTION,
    INITIALIZER,
  };

private:
  ErrorHandler &errorHandler_;
  FunctionType currentFunction_;
  std::vector<std::map<std::string, bool>> scopes_;

public:
  Resolver(ErrorHandler &errorHandler)
      : errorHandler_(errorHandler),
        currentFunction_(NONEF)
  {}

  ~Resolver() = default;
  void resolve(const std::vector<std::shared_ptr<Stmt>> &prog);

private:
  void resolve(std::shared_ptr<Stmt> stmt);
  void resolve(std::shared_ptr<Expr> expr);
  void resolveDecls(const std::vector<std::shared_ptr<Decl>>& decls);
  void resolveLocal(std::shared_ptr<Expr> expr, const Token &tok);
  void resolveFunction(std::shared_ptr<Function> fn, FunctionType type);

  void beginScope();
  void endScope();
  void declare(const Token &tok);
  void define(const Token &name);

  virtual std::any visitFunction(std::shared_ptr<Function> stmt) override;
  virtual std::any visitExpression(std::shared_ptr<Expression> stmt) override;
  virtual std::any visitReturn(std::shared_ptr<Return> stmt) override;
  virtual std::any visitDecl(std::shared_ptr<Decl> stmt) override;
  virtual std::any visitBlock(std::shared_ptr<Block> stmt) override;
  virtual std::any visitAssign(std::shared_ptr<Assign> expr) override;
  virtual std::any visitBinaryExpr(std::shared_ptr<BinaryExpr> expr) override;
  virtual std::any visitLiteralExpr(std::shared_ptr<LiteralExpr> expr) override;
  virtual std::any visitUnaryExpr(std::shared_ptr<UnaryExpr> expr) override;
  virtual std::any visitVariable(std::shared_ptr<Variable> expr) override;
};

} // namespace ccomp

#endif