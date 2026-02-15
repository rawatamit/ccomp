#ifndef TYPE_RESOLVER_HPP
#define TYPE_RESOLVER_HPP

#include "Type.h"
#include "ast/Expr.h"
#include "ast/Stmt.h"

namespace ccomp {
class ErrorHandler;

class TypeResolver {
public:
  TypeResolver(ErrorHandler& errorHandler);
  void resolve(const std::vector<std::unique_ptr<Stmt>>& prog);

private:
  ErrorHandler& errorHandler_;
  std::unordered_map<std::string, const Type*> tytab_;

  const Type* gettype(Expr* expr);
  const Type* gettype(Stmt* stmt);

public:
  const Type* operator()(const Block& stmt);
  const Type* operator()(const Expression& stmt);
  const Type* operator()(Function& stmt);
  const Type* operator()(FunctionParam&);
  const Type* operator()(const If& stmt);
  const Type* operator()(const Return& stmt);
  const Type* operator()(const DoWhile& Stmt);
  const Type* operator()(const While& stmt);
  const Type* operator()(const For& Stmt);
  const Type* operator()(const Decl& stmt);
  const Type* operator()(const Null& stmt);
  const Type* operator()(const Break& stmt);
  const Type* operator()(const Continue& stmt);

  const Type* operator()(const Assign& expr);
  const Type* operator()(const Conditional& expr);
  const Type* operator()(const BinaryExpr& expr);
  const Type* operator()(const LiteralExpr& expr);
  const Type* operator()(const UnaryExpr& expr);
  const Type* operator()(const Variable& expr);
  const Type* operator()(const Call& expr);
};
} // namespace ccomp

#endif // TYPE_RESOLVER_HPP