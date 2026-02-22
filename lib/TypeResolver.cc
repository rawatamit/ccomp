#include "TypeResolver.h"
#include "ErrorHandler.h"
#include "Util.h"
#include <cassert>

using namespace ccomp;

TypeResolver::TypeResolver(ErrorHandler& errorHandler) :
  errorHandler_(errorHandler)
{}

const TypeResolver::TypeTable& TypeResolver::typecheck(const std::vector<std::unique_ptr<Stmt>>& prog) {
  for (auto& stmt : prog) {
    typecheck(stmt.get());
  }

  return symtab_;
}

const Type* TypeResolver::typecheck(Expr* expr) {
  return std::visit(*this, *expr);
}

const Type* TypeResolver::typecheck(Stmt* stmt) {
  return std::visit(*this, *stmt);
}

void TypeResolver::add(const std::string& name, std::shared_ptr<Symbol> sym,
                       const Type* ty, std::unique_ptr<SymbolAttrs> attrs) {
  sym->setType(ty);
  sym->setAttrs(std::move(attrs));
  symtab_[name] = sym;
}

const Type* TypeResolver::operator()(const Block& block) {
  for (auto& stmt : block.stmts) {
    typecheck(stmt.get());
  }

  return nullptr;
}

const Type* TypeResolver::operator()(const Expression& stmt) {
  return typecheck(stmt.expr.get());
}

const Type* TypeResolver::operator()(Function& fn) {
  std::vector<const Type*> paramTypes;
  for (auto& param : fn.params) {
    paramTypes.emplace_back(typecheck(param.get()));
  }

  auto fnstr = fn.sym->getName();
  if (!fn.fileScope && (fn.storage == Scope::STORAGE_STATIC)) {
    errorHandler_.add(0,
                      " function " + fnstr,
                      "Static function declaration inside local scope.");
  }

  bool isGlobal = (fn.storage != Scope::STORAGE_STATIC);
  bool alreadyDefined = false;

  // NOTE: all functions return an integer.
  const FunctionType* fty =
    new FunctionType(BuiltInType::getInt32Ty(), paramTypes);
  fn.evalty = std::make_unique<FunctionType>(
    FunctionType(BuiltInType::getInt32Ty(), paramTypes));

  auto oldDecl = symtab_.find(fnstr);
  if (oldDecl != symtab_.end()) {
    // Function in symbol table.
    const Type* oldty = oldDecl->second->getType();
    if (auto oldfty = dynamic_cast<const FunctionType*>(oldty)) {
      if (*oldfty != *fty) {
        errorHandler_.add(0,
                          " function " + fnstr,
                          "Redeclaration with different type.");
      }
    } else {
      errorHandler_.add(0,
                        " function " + fnstr,
                        "Redeclaration as function.");
    }

    bool isOldDeclGlobal = oldDecl->second->getAttrs()->isGlobal();
    if (isOldDeclGlobal && (fn.storage == Scope::STORAGE_STATIC)) {
      errorHandler_.add(0,
                        " function " + fnstr,
                        "Redeclaration with different storage class.");
    }

    // if a previous declaration was static, and this declaration is extern,
    // this function is viewed as static
    isGlobal = isOldDeclGlobal;
    // has this function already defined
    alreadyDefined = oldDecl->second->getAttrs()->isDefined();
  }

  add(fnstr, fn.sym, fn.evalty.get(),
      std::make_unique<SymbolAttrs>(alreadyDefined || fn.body, isGlobal));

  if (auto& body = fn.body) {
    typecheck(body.get());
  }

  return fty;
}

const Type* TypeResolver::operator()(FunctionParam& param) {
  assert(param.type.type == TokenType::INT);
  param.evalty = BuiltInType::getInt32Ty();
  return param.evalty;
}

const Type* TypeResolver::operator()(const If& ifstmt) {
  auto condty = typecheck(ifstmt.condition.get());
  if ((condty != BuiltInType::getBoolTy()) &&
      (condty != BuiltInType::getInt32Ty())) {
    errorHandler_.add(0,
                      " condition in if",
                      "Boolean type expected.");
  }

  typecheck(ifstmt.thenBranch.get());
  if (auto& elseExp = ifstmt.elseBranch) {
    typecheck(elseExp.get());
  }

  return nullptr;
}

const Type* TypeResolver::operator()(const Return& stmt) {
  return typecheck(stmt.value.get());
}

const Type* TypeResolver::operator()(const DoWhile& loop) {
  if (auto& cond = loop.condition) {
    typecheck(cond.get());
  }

  typecheck(loop.body.get());
  return nullptr;
}

const Type* TypeResolver::operator()(const While& loop) {
  if (auto& cond = loop.condition) {
    typecheck(cond.get());
  }

  typecheck(loop.body.get());
  return nullptr;
}

const Type* TypeResolver::operator()(const For& loop) {
  if (auto& init = loop.init) {
    typecheck(init.get());
  }

  if (auto& cond = loop.condition) {
    typecheck(cond.get());
  }

  if (auto& post = loop.post) {
    typecheck(post.get());
  }

  typecheck(loop.body.get());
  return nullptr;
}

const Type* TypeResolver::typecheckFileScopeDecl(const Decl& decl) {
  const Variable* var = std::get_if<Variable>(decl.name.get());
  std::string declstr = var->sym->getName();
  const Type* ty = BuiltInType::getInt32Ty();
  InitialValue initValue;

  if (auto& init = decl.init) {
    if (auto constval = std::get_if<LiteralExpr>(init.get())) {
      initValue = InitialValue(InitialValue::INITIAL_VALUE,
                                std::stoi(constval->value));
    } else {
      errorHandler_.add(0,
                        " at declaration",
                        "Non-const initializer.");
    }
  } else {
    if (decl.storage == Scope::STORAGE_EXTERN) {
      initValue = InitialValue(InitialValue::NOINIT_VALUE);
    } else {
      initValue = InitialValue(InitialValue::TENTATIVE_VALUE);
    }
  }

  bool isGlobal = (decl.storage != Scope::STORAGE_STATIC);
  auto oldDecl = symtab_.find(declstr);

  if (oldDecl != symtab_.end()) {
    const Type* oldty = oldDecl->second->getType();
    // old type is not Int
    if (oldty != BuiltInType::getInt32Ty()) {
      errorHandler_.add(0,
                        " declaration " + declstr,
                        "Function redeclared as variable.");
    }

    // if a previous declaration was static, and this declaration is extern,
    // this declaration is viewed as static
    bool isOldDeclGlobal = oldDecl->second->getAttrs()->isGlobal();
    if (decl.storage == Scope::STORAGE_EXTERN) {
      isGlobal = isOldDeclGlobal;
    } else if (isOldDeclGlobal != isGlobal) {
      errorHandler_.add(0,
                        " declaration " + declstr,
                        "Conflicting linkage for declaration.");
    }

    InitialValue oldInitValue = oldDecl->second->getAttrs()->getInitValue();
    if (oldInitValue.getType() == InitialValue::INITIAL_VALUE) {
      if (initValue.getType() == InitialValue::INITIAL_VALUE) {
        errorHandler_.add(0,
                          " declaration " + declstr,
                          "Conflicting file scope variable definitions.");
      } else {
        initValue = oldInitValue;
      }
    } else if ((initValue.getType() != InitialValue::INITIAL_VALUE) &&
                (oldInitValue.getType() == InitialValue::TENTATIVE_VALUE)) {
      initValue = InitialValue(InitialValue::TENTATIVE_VALUE);
    }
  }

  add(declstr, var->sym, ty,
      std::make_unique<SymbolAttrs>(initValue, isGlobal));
  return ty;
}

const Type* TypeResolver::typecheckLocalDecl(const Decl& decl) {
  const Variable* var = std::get_if<Variable>(decl.name.get());
  std::string declstr = var->sym->getName();
  const Type* ty = BuiltInType::getInt32Ty();

  if (decl.storage == Scope::STORAGE_EXTERN) {
    if (decl.init) {
      errorHandler_.add(0,
                        " declaration " + declstr,
                        "Local extern definition has initializer.");
    }

    auto oldDecl = symtab_.find(declstr);
    if (oldDecl != symtab_.end()) {
      const Type* oldty = oldDecl->second->getType();
      // old type is a function type
      if (oldty != BuiltInType::getInt32Ty()) {
        errorHandler_.add(0,
                          " declaration " + declstr,
                          "Function redeclared as variable.");
      }
    } else {
      add(declstr, var->sym, ty,
          std::make_unique<SymbolAttrs>(
          InitialValue(InitialValue::NOINIT_VALUE), true));
    }
  } else if (decl.storage == Scope::STORAGE_STATIC) {
    if (decl.loopDecl) {
      errorHandler_.add(0,
                        " at declaration",
                        "For loop initializer can't be declared static.");
    }

    InitialValue initValue;
    if (decl.init == nullptr) {
      initValue = InitialValue(0);
    } else if (auto constval = std::get_if<LiteralExpr>(decl.init.get())) {
      initValue = InitialValue(InitialValue::INITIAL_VALUE,
                                std::stoi(constval->value));
    } else {
      errorHandler_.add(0,
                        " at declaration",
                        "Non-const initializer on local static variable.");
    }

    add(declstr, var->sym, ty,
        std::make_unique<SymbolAttrs>(initValue, false));
  } else {
    add(declstr, var->sym, ty,
        std::make_unique<SymbolAttrs>());

    if (auto& init = decl.init) {
      typecheck(init.get());
    }
  }

  return ty;
}

const Type* TypeResolver::operator()(const Decl& decl) {
  return (decl.fileScope) ?
         typecheckFileScopeDecl(decl) :
         typecheckLocalDecl(decl);
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
  auto lty = typecheck(assign.lvalue.get());
  auto vty = typecheck(assign.value.get());

  if (lty != vty) {
    errorHandler_.add(0,
                      " at assignment",
                      "Type mismatch between lvalue and rvalue.");
  }

  return lty;
}

const Type* TypeResolver::operator()(const Conditional& cexpr) {
  auto condty = typecheck(cexpr.condition.get());
  if ((condty != BuiltInType::getBoolTy()) &&
      (condty != BuiltInType::getInt32Ty())) {
    errorHandler_.add(0,
                      " at condition in ternary",
                      "Boolean type expected.");
  }

  auto thenty = typecheck(cexpr.thenExp.get());
  auto elsety = typecheck(cexpr.elseExp.get());
  if (thenty != elsety) {
    errorHandler_.add(0,
                      " at expression in ternary",
                      "Expression types in ternary must match.");
  }

  return thenty;
}

const Type* TypeResolver::operator()(const BinaryExpr& expr) {
  auto tyleft = typecheck(expr.left.get());
  auto tyright = typecheck(expr.right.get());
  if (tyleft != tyright) {
    errorHandler_.add(expr.Operator.line,
                      " at expression " + expr.Operator.toString(),
                      "Type mismatch.");
  }

  return BuiltInType::getInt32Ty();
}

const Type* TypeResolver::operator()(const LiteralExpr& expr) {
  assert(expr.type == TokenType::NUMBER);
  return BuiltInType::getInt32Ty();
}

const Type* TypeResolver::operator()(const UnaryExpr& expr) {
  return typecheck(expr.right.get());
}

const Type* TypeResolver::operator()(const Variable& var) {
  auto varstr = var.sym->getName();
  auto resolvedRef = symtab_.find(varstr);
  const Type* ty = (resolvedRef != symtab_.end()) ? resolvedRef->second->getType() : nullptr;

  if (ty != nullptr) {
    return ty;
  } else if (auto fn = var.sym->getFunction()) {
    // Note: calling gettype again on function, will trigger type recomputation.
    ty = fn->evalty.get();
  } else if (auto param = var.sym->getFunctionParam()) {
    ty = param->evalty;
  } else {
    ty = BuiltInType::getInt32Ty();
  }

  return ty;
}

const Type* TypeResolver::operator()(const Call& call) {
  const Function* fn = call.fn;
  if (fn->params.size() != call.args.size()) {
    errorHandler_.add(fn->name.line, " at '" + fn->name.toString() + "'",
                      "Call and function args don't match.");
  }

  return fn->evalty->getReturnTy();
}
