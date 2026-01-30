#ifndef _RESOLVER_H_
#define _RESOLVER_H_

#include "ErrorHandler.h"
#include "Token.h"
#include "ast/Expr.h"
#include "ast/Stmt.h"

#include <map>
#include <vector>

namespace ccomp {

class Resolver {
private:
  enum FunctionType {
    NONEF,
    FUNCTION,
    INITIALIZER,
  };

  ErrorHandler &errorHandler_;
  FunctionType currentFunction_;
  std::vector<std::map<std::string, bool>> scopes_;

public:
  Resolver(ErrorHandler &errorHandler)
      : errorHandler_(errorHandler),
        currentFunction_(NONEF)
  {}

  ~Resolver() = default;
  void resolve(const std::vector<std::unique_ptr<Stmt>>& prog);

private:
  void resolve(Stmt* stmt);
  void resolve(Expr* expr);
  void resolveDecls(const std::vector<std::unique_ptr<Decl>>& decls);
  void resolveLocal(std::unique_ptr<Expr> expr, const Token& tok);
  void resolveFunction(const Function& fn, FunctionType type);

  void beginScope();
  void endScope();
  void declare(const Token &tok);
  void define(const Token &name);

public:
  void operator()(const Block& stmt);
  void operator()(const Expression& stmt);
  void operator()(const Function& stmt);
  void operator()(const If& stmt);
  void operator()(const Return& stmt);
  void operator()(const While& stmt);
  void operator()(const Decl& stmt);
  void operator()(const Null& stmt);

  void operator()(const Assign& expr);
  void operator()(const Conditional& expr);
  void operator()(const BinaryExpr& expr);
  void operator()(const LiteralExpr& expr);
  void operator()(const UnaryExpr& expr);
  void operator()(const Variable& expr);
};

} // namespace ccomp

#endif