#include "TypeResolver.h"
#include "ErrorHandler.h"
#include "Util.h"
#include <cassert>

using namespace ccomp;

TypeResolver::TypeResolver(ErrorHandler& errorHandler) :
  errorHandler_(errorHandler)
{}

void TypeResolver::resolve(const std::vector<std::unique_ptr<Stmt>>& prog) {
  for (auto& stmt : prog) {
    gettype(stmt.get());
  }
}

const Type* TypeResolver::gettype(Expr* expr) {
  return std::visit(*this, *expr);
}

const Type* TypeResolver::gettype(Stmt* stmt) {
  return std::visit(*this, *stmt);
}

const Type* TypeResolver::operator()(const Block& block) {
  for (auto& stmt : block.stmts) {
    gettype(stmt.get());
  }

  return nullptr;
}

const Type* TypeResolver::operator()(const Expression& stmt) {
  return gettype(stmt.expr.get());
}

const Type* TypeResolver::operator()(Function& fn) {
  std::vector<const Type*> paramTypes;
  for (auto& param : fn.params) {
    //auto param_stmt = Stmt(param);
    paramTypes.emplace_back(gettype(param.get()));
  }

  // NOTE: all functions return an integer.
  fn.evalty = std::make_unique<FunctionType>(BuiltInType::getInt32Ty(), paramTypes);

  // Is function already in symbol table?
  auto fnstr = toStr(fn);
  auto fnit = tytab_.find(fnstr);
  const Type* oldty = (fnit != tytab_.end()) ? fnit->second : nullptr;
  if (oldty) {
    if (auto fty = dynamic_cast<const FunctionType*>(oldty)) {
      if (*fty != *fn.evalty.get()) {
        errorHandler_.add(0,
                          " function type " + fnstr,
                          "Redeclaration with different type.");
      }
    } else {
      errorHandler_.add(0,
                        " function type " + fnstr,
                        "Redeclaration as function.");
    }
  } else {
    tytab_[fnstr] = fn.evalty.get();
  }

  if (auto& body = fn.body) {
    gettype(body.get());
  }

  return fn.evalty.get();
}

const Type* TypeResolver::operator()(FunctionParam& param) {
  assert(param.type.type == TokenType::INT);
  param.evalty = BuiltInType::getInt32Ty();
  return param.evalty;
}

const Type* TypeResolver::operator()(const If& ifstmt) {
  auto condty = gettype(ifstmt.condition.get());
  if ((condty != BuiltInType::getBoolTy()) &&
      (condty != BuiltInType::getInt32Ty())) {
    errorHandler_.add(0,
                      " condition in if",
                      "Boolean type expected.");
  }

  gettype(ifstmt.thenBranch.get());
  if (auto& elseExp = ifstmt.elseBranch) {
    gettype(elseExp.get());
  }
  return nullptr;
}

const Type* TypeResolver::operator()(const Return& stmt) {
  return gettype(stmt.value.get());
}

const Type* TypeResolver::operator()(const DoWhile& loop) {
  if (auto& cond = loop.condition) {
    gettype(cond.get());
  }

  gettype(loop.body.get());
  return nullptr;
}

const Type* TypeResolver::operator()(const While& loop) {
  if (auto& cond = loop.condition) {
    gettype(cond.get());
  }

  gettype(loop.body.get());
  return nullptr;
}

const Type* TypeResolver::operator()(const For& loop) {
  if (auto& init = loop.init) {
    gettype(init.get());
  }

  if (auto& cond = loop.condition) {
    gettype(cond.get());
  }

  if (auto& post = loop.post) {
    gettype(post.get());
  }

  gettype(loop.body.get());
  return nullptr;
}

const Type* TypeResolver::operator()(const Decl& decl) {
  auto namety = gettype(decl.name.get());
  if (auto& init = decl.init) {
    auto initty = gettype(init.get());
    if (namety != initty) {
      errorHandler_.add(0,
                        " at declaration",
                        "Type mismatch between name and init.");
    }
  }

  return namety;
}

const Type* TypeResolver::operator()(const Null&) {
  return nullptr;
}

const Type* TypeResolver::operator()(const Break&) {
  return nullptr;
}

const Type* TypeResolver::operator()(const Continue&) {
  return nullptr;
}

const Type* TypeResolver::operator()(const Assign& assign) {
  auto lty = gettype(assign.lvalue.get());
  auto vty = gettype(assign.value.get());

  if (lty != vty) {
    errorHandler_.add(0,
                      " at assignment",
                      "Type mismatch between lvalue and rvalue.");
  }

  return lty;
}

const Type* TypeResolver::operator()(const Conditional& cexpr) {
  auto condty = gettype(cexpr.condition.get());
  if ((condty != BuiltInType::getBoolTy()) &&
      (condty != BuiltInType::getInt32Ty())) {
    errorHandler_.add(0,
                      " at condition in ternary",
                      "Boolean type expected.");
  }

  auto thenty = gettype(cexpr.thenExp.get());
  auto elsety = gettype(cexpr.elseExp.get());
  if (thenty != elsety) {
    errorHandler_.add(0,
                      " at expression in ternary",
                      "Expression types in ternary must match.");
  }

  return thenty;
}

const Type* TypeResolver::operator()(const BinaryExpr& expr) {
  auto tyleft = gettype(expr.left.get());
  auto tyright = gettype(expr.right.get());
  if (tyleft != tyright) {
    errorHandler_.add(expr.Operator.line,
                      " at expression " + expr.Operator.toString(),
                      "Type mismatch.");
  }

  #if 0
  auto optype = expr.Operator.type;
  if (isLogicalOp(optype) || isRelationalOp(optype)) {
    return BuiltInType::getBoolTy();
  } else {
    return BuiltInType::getInt32Ty();
  }
  #endif
  return BuiltInType::getInt32Ty();
}

const Type* TypeResolver::operator()(const LiteralExpr& expr) {
  assert(expr.type == TokenType::NUMBER);
  return BuiltInType::getInt32Ty();
}

const Type* TypeResolver::operator()(const UnaryExpr& expr) {
  return gettype(expr.right.get());
}

const Type* TypeResolver::operator()(const Variable& var) {
  auto varstr = toStr(var);
  auto it = tytab_.find(varstr);
  const Type* ty = (it != tytab_.end()) ? it->second : nullptr;

  if (ty != nullptr) {
    return ty;
  } else if (var.var.type() == typeid(Function*)) {
    Function* fn = std::any_cast<Function*>(var.var);
    // Note: calling gettype again on function, will trigger type recomputation.
    ty = fn->evalty.get();
  } else if (var.var.type() == typeid(FunctionParam*)) {
    FunctionParam* param = std::any_cast<FunctionParam*>(var.var);
    ty = param->evalty;
  } else {
    ty = BuiltInType::getInt32Ty();
  }

  tytab_[varstr] = ty;
  return ty;
}

const Type* TypeResolver::operator()(const Call& call) {
  Function* fn = call.fn;
  if (fn->params.size() != call.args.size()) {
    errorHandler_.add(fn->name.line, " at '" + fn->name.toString() + "'",
                      "Call and function args don't match.");
  }

  return fn->evalty->getReturnTy();
}
