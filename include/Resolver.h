#ifndef _RESOLVER_H_
#define _RESOLVER_H_

#include "ErrorHandler.h"
#include "Token.h"
#include "ast/Expr.h"
#include "ast/Stmt.h"

#include <vector>
#include <unordered_map>

namespace ccomp {
class Resolver {
private:
  enum FunctionType {
    NONEF,
    FUNCTION,
    INITIALIZER,
  };

  ErrorHandler& errorHandler_;
  FunctionType currentFunction_;
  std::vector<std::unordered_map<std::string, bool>> scopes_;
  std::vector<int> nested_loop_labels_;
  int loop_label_;

public:
  Resolver(ErrorHandler& errorHandler)
    : errorHandler_(errorHandler),
      currentFunction_(NONEF),
      loop_label_(0)
  {}

  ~Resolver() = default;
  void resolve(const std::vector<std::unique_ptr<Stmt>>& prog);

private:
  void resolve(Stmt* stmt);
  void resolve(Expr* expr);
  int resolveLocal(const Token& tok);
  void resolveFunction(const Function& fn, FunctionType type);

  void beginScope();
  void endScope();
  void declare(const Token &tok);
  void define(const Token &name);

  void beginLoop(int* label);
  void endLoop();
  void copyLoopLabel(int* label);

public:
  void operator()(const Block& stmt);
  void operator()(const Expression& stmt);
  void operator()(const Function& stmt);
  void operator()(const If& stmt);
  void operator()(const Return& stmt);
  void operator()(DoWhile& Stmt);
  void operator()(While& stmt);
  void operator()(For& Stmt);
  void operator()(const Decl& stmt);
  void operator()(const Null& stmt);
  void operator()(Break& stmt);
  void operator()(Continue& stmt);

  void operator()(const Assign& expr);
  void operator()(const Conditional& expr);
  void operator()(const BinaryExpr& expr);
  void operator()(const LiteralExpr& expr);
  void operator()(const UnaryExpr& expr);
  void operator()(Variable& expr);
};

} // namespace ccomp

#endif