#ifndef _RESOLVER_H_
#define _RESOLVER_H_

#include "ErrorHandler.h"
#include "Token.h"
#include "ast/Expr.h"
#include "ast/Stmt.h"
#include <vector>

namespace ccomp {
class Scope;

class Resolver {
private:
  enum FunctionType {
    NONEF,
    FUNCTION,
  };

  ErrorHandler& errorHandler_;
  FunctionType currentFunction_;
  std::shared_ptr<Scope> globalScope_;
  std::shared_ptr<Scope> curScope_;
  std::vector<int> nested_loop_labels_;
  int loop_label_;
  static int uniqueId_;

public:
  Resolver(ErrorHandler& errorHandler);
  ~Resolver() = default;
  void resolve(const std::vector<std::unique_ptr<Stmt>>& prog);

private:
  void resolve(Stmt* stmt);
  void resolve(Expr* expr);
  void resolveFunction(Function& fn, FunctionType type);
  Function* getFunction(const SymbolTable& detail) const;

  void beginScope();
  void endScope();
  void declare(const Token& name, Function* fn);

  void beginLoop(int* label);
  void endLoop();
  void copyLoopLabel(int* label);

  std::string getUniqueName(const Decl& decl, bool hasExternalLinkage);
  std::string getUniqueName(const Function& fn);
  std::string getUniqueName(const FunctionParam& param);

public:
  void operator()(const Block& stmt);
  void operator()(const Expression& stmt);
  void operator()(Function& stmt);
  void operator()(FunctionParam&);
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
  void operator()(Call& expr);
};

} // namespace ccomp

#endif