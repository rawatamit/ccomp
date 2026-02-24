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
  SymTabT& typecheck(const std::vector<std::unique_ptr<Stmt>>& prog);

private:
  ErrorHandler& errorHandler_;
  SymTabT symtab_;

  const Type* typecheck(Expr* expr);
  const Type* typecheck(Stmt* stmt);
  const Type* typecheckFileScopeDecl(Decl& decl);
  const Type* typecheckLocalDecl(Decl& decl);

  void add(const std::string& name, std::shared_ptr<Symbol> sym, const Type* ty,
           std::unique_ptr<SymbolAttrs> attrs);

public:
  const Type* operator()(const Block& stmt);
  const Type* operator()(const Expression& stmt);
  const Type* operator()(Function& stmt);
  const Type* operator()(FunctionParam&);
  const Type* operator()(const If& stmt);
  const Type* operator()(Return& stmt);
  const Type* operator()(const DoWhile& Stmt);
  const Type* operator()(const While& stmt);
  const Type* operator()(const For& Stmt);
  const Type* operator()(Decl& stmt);
  const Type* operator()(const Null& stmt);
  const Type* operator()(const Break& stmt);
  const Type* operator()(const Continue& stmt);
  const Type* operator()(Assign& expr);
  const Type* operator()(Conditional& expr);
  const Type* operator()(BinaryExpr& expr);
  const Type* operator()(Int32Exp& expr);
  const Type* operator()(Int64Exp& expr);
  const Type* operator()(const StringExp& expr);
  const Type* operator()(CastExpr& expr);
  const Type* operator()(UnaryExpr& expr);
  const Type* operator()(Variable& expr);
  const Type* operator()(Call& expr);
};
} // namespace ccomp

#endif // TYPE_RESOLVER_HPP