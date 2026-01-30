#ifndef TACKYGEN_H
#define TACKYGEN_H

#include "ErrorHandler.h"
#include "ast/Tacky.h"
#include "ast/Expr.h"
#include "ast/Stmt.h"

namespace ccomp {
class TackyGen {
public:
  TackyGen(const std::vector<std::unique_ptr<Stmt>>& stmts, ErrorHandler& errorHandler);
  std::shared_ptr<Tacky> gen();

private:
  const std::vector<std::unique_ptr<Stmt>>& stmts_;
  std::vector<std::shared_ptr<Tacky>> instructions_;
  ErrorHandler& errorHandler_;

  std::shared_ptr<Tacky> gen(Expr* expr);
  std::shared_ptr<Tacky> gen(Stmt* stmt);
  void gen(const std::vector<std::unique_ptr<Stmt>>& stmts);

  std::shared_ptr<Tacky> genLogical(const BinaryExpr& expr);
  std::string unique_var();
  std::string unique_label(const std::string& desc);

  template<typename T, typename... Args>
  std::shared_ptr<Tacky> make_tacky(Args&&... args)
  { return std::make_shared<Tacky>(T(std::forward<Args>(args)...)); }

public:
  std::shared_ptr<Tacky> operator()(const Block& stmt);
  std::shared_ptr<Tacky> operator()(const Expression& stmt);
  std::shared_ptr<Tacky> operator()(const Function& stmt);
  std::shared_ptr<Tacky> operator()(const If& stmt);
  std::shared_ptr<Tacky> operator()(const Return& Stmt);
  std::shared_ptr<Tacky> operator()(const While& Stmt);
  std::shared_ptr<Tacky> operator()(const Decl& Stmt);
  std::shared_ptr<Tacky> operator()(const Assign& expr);
  std::shared_ptr<Tacky> operator()(const Null& expr);

  std::shared_ptr<Tacky> operator()(const Conditional& expr);
  std::shared_ptr<Tacky> operator()(const BinaryExpr& expr);
  std::shared_ptr<Tacky> operator()(const LiteralExpr& expr);
  std::shared_ptr<Tacky> operator()(const UnaryExpr& expr);
  std::shared_ptr<Tacky> operator()(const Variable& expr);
};
}

#endif // TACKYGEN_H
