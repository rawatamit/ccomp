#include "TypeResolver.h"
#include "ErrorHandler.h"
#include "Util.h"
#include <cassert>

using namespace ccomp;

static const Type* sGetCommonType(const Type* a, const Type* b) {
  if (a == b) {
    return a;
  } else {
    return BuiltInType::getInt64Ty();
  }
}

static std::unique_ptr<Expr> sConvertTo(std::unique_ptr<Expr> expr, const Type* origTy, const Type* promoteTy) {
  if (origTy == promoteTy) {
    return expr;
  } else {
    // FIXME!!!
    auto ret = std::make_unique<Expr>(CastExpr(Token(TokenType::ERROR, "lexeme", "lit", 0), std::move(expr)));
    auto cexpr = std::get_if<CastExpr>(ret.get());
    cexpr->evalty = promoteTy;
    return ret;
  }
}

TypeResolver::TypeResolver(ErrorHandler& errorHandler) :
  errorHandler_(errorHandler)
{}

SymTabT& TypeResolver::typecheck(const std::vector<std::unique_ptr<Stmt>>& prog) {
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

  const std::string& fnstr = fn.sym->getName();
  if (!fn.fileScope && (fn.storage == Scope::STORAGE_STATIC)) {
    errorHandler_.add(0,
                      " function " + fnstr,
                      "Static function declaration inside local scope.");
  }

  bool isGlobal = (fn.storage != Scope::STORAGE_STATIC);
  bool alreadyDefined = false;
  const Type* fnty =
    new FunctionType(FunctionType(getTypeFromToken(fn.returnty), paramTypes));

  auto oldDecl = symtab_.find(fnstr);
  if (oldDecl != symtab_.end()) {
    // Function in symbol table.
    const Type* oldty = oldDecl->second->getType();
    if (auto oldfty = dynamic_cast<const FunctionType*>(oldty)) {
      if (*oldfty != *static_cast<const FunctionType*>(fnty)) {
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

  // set type definition to function symbol.
  fn.sym->setType(fnty);

  // update symbol table
  add(fnstr, fn.sym, fnty,
      std::make_unique<SymbolAttrs>(alreadyDefined || fn.body, isGlobal));

  if (auto& body = fn.body) {
    typecheck(body.get());
  }

  return fnty;
}

const Type* TypeResolver::operator()(FunctionParam& param) {
  const Type* ty = getTypeFromToken(param.type);
  param.sym->setType(ty);
  return ty;
}

const Type* TypeResolver::operator()(const If& ifstmt) {
  auto condty = typecheck(ifstmt.condition.get());
  if ((condty != BuiltInType::getInt32Ty()) &&
      (condty != BuiltInType::getInt64Ty())) {
    errorHandler_.add(0,
                      " condition in if",
                      "Integer type expected.");
  }

  typecheck(ifstmt.thenBranch.get());
  if (auto& elseExp = ifstmt.elseBranch) {
    typecheck(elseExp.get());
  }

  return nullptr;
}

const Type* TypeResolver::operator()(Return& stmt) {
  const Type* ty = stmt.fnReturnTy;
  const Type* valuety = typecheck(stmt.value.get());
  stmt.value = sConvertTo(std::move(stmt.value), valuety, ty);
  return ty;
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

const Type* TypeResolver::typecheckFileScopeDecl(Decl& decl) {
  const Variable* var = std::get_if<Variable>(decl.name.get());
  const std::string& declstr = var->sym->getName();
  const Type* ty = getTypeFromToken(decl.type);
  InitialValue initValue;

  if (auto& init = decl.init) {
    if (auto constval = std::get_if<Int32Exp>(init.get())) {
      initValue = InitialValue(InitialValue::INITIAL_INT32_VALUE, constval->int32);
    } else if (auto constval = std::get_if<Int64Exp>(init.get())) {
      initValue = InitialValue(InitialValue::INITIAL_LONG_VALUE, constval->int64);
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
    //if (oldty != BuiltInType::getInt32Ty()) {
    if (dynamic_cast<const FunctionType*>(oldty)) {
      errorHandler_.add(0,
                        " declaration " + declstr,
                        "Function redeclared as variable.");
    } else if (oldty != ty) {
      errorHandler_.add(0,
                        " declaration " + declstr,
                        "Variable redeclared with different type.");
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
    if (oldInitValue.getType() == InitialValue::INITIAL_INT32_VALUE ||
        oldInitValue.getType() == InitialValue::INITIAL_LONG_VALUE) {
      if (initValue.getType() == InitialValue::INITIAL_INT32_VALUE ||
          initValue.getType() == InitialValue::INITIAL_LONG_VALUE) {
        errorHandler_.add(0,
                          " declaration " + declstr,
                          "Conflicting file scope variable definitions.");
      } else {
        initValue = oldInitValue;
      }
    } else if (((initValue.getType() != InitialValue::INITIAL_INT32_VALUE) &&
                (initValue.getType() != InitialValue::INITIAL_LONG_VALUE)) &&
               (oldInitValue.getType() == InitialValue::TENTATIVE_VALUE)) {
      initValue = InitialValue(InitialValue::TENTATIVE_VALUE);
    }
  }

  add(declstr, var->sym, ty,
      std::make_unique<SymbolAttrs>(initValue, isGlobal));
  return ty;
}

const Type* TypeResolver::typecheckLocalDecl(Decl& decl) {
  const Variable* var = std::get_if<Variable>(decl.name.get());
  const std::string& declstr = var->sym->getName();
  const Type* ty = getTypeFromToken(decl.type);

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
      //if (oldty != BuiltInType::getInt32Ty()) {
      if (dynamic_cast<const FunctionType*>(oldty)) {
        errorHandler_.add(0,
                          " declaration " + declstr,
                          "Function redeclared as variable.");
      } else if (oldty != ty) {
        errorHandler_.add(0,
                          " declaration " + declstr,
                          "Variable redeclared with different type.");
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
    } else if (auto constval = std::get_if<Int32Exp>(decl.init.get())) {
      initValue = InitialValue(InitialValue::INITIAL_INT32_VALUE, constval->int32);
    } else if (auto constval = std::get_if<Int64Exp>(decl.init.get())) {
      initValue = InitialValue(InitialValue::INITIAL_LONG_VALUE, constval->int64);
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
      const Type* initty = typecheck(init.get());
      decl.init = sConvertTo(std::move(init), initty, ty);
    }
  }

  return ty;
}

const Type* TypeResolver::operator()(Decl& decl) {
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

const Type* TypeResolver::operator()(Assign& assign) {
  auto lty = typecheck(assign.lvalue.get());
  auto vty = typecheck(assign.value.get());
  if (dynamic_cast<const FunctionType*>(lty) ||
      dynamic_cast<const FunctionType*>(vty)) {
    errorHandler_.add(0, " assignment",
                      "Function type in assignment.");
  }

  const Type* promoteTy = sGetCommonType(lty, vty);
  assign.value = sConvertTo(std::move(assign.value), vty, promoteTy);
  return assign.evalty = lty;
}

const Type* TypeResolver::operator()(Conditional& cexpr) {
  typecheck(cexpr.condition.get());
  auto thenty = typecheck(cexpr.thenExp.get());
  auto elsety = typecheck(cexpr.elseExp.get());
  const Type* commonty = sGetCommonType(thenty, elsety);
  cexpr.thenExp = sConvertTo(std::move(cexpr.thenExp), thenty, commonty);
  cexpr.elseExp = sConvertTo(std::move(cexpr.elseExp), elsety, commonty);
  return cexpr.evalty = commonty;
}

const Type* TypeResolver::operator()(BinaryExpr& binexpr) {
  auto leftty = typecheck(binexpr.left.get());
  auto rightty = typecheck(binexpr.right.get());

  if (dynamic_cast<const FunctionType*>(leftty) ||
      dynamic_cast<const FunctionType*>(rightty)) {
    errorHandler_.add(0, " binary expression",
                      "Function type operand in expression.");
  }

  // || and && resolve to integer type
  TokenType optype = binexpr.op.type;
  if ((optype == TokenType::AMPERSAND_AMPERSAND) ||
      (optype == TokenType::PIPE_PIPE)) {
    return binexpr.evalty = BuiltInType::getInt32Ty();
  }

  const Type* commonty = sGetCommonType(leftty, rightty);
  binexpr.left = sConvertTo(std::move(binexpr.left), leftty, commonty);
  binexpr.right = sConvertTo(std::move(binexpr.right), rightty, commonty);

  if ((optype == TokenType::PLUS) || (optype == TokenType::MINUS) ||
      (optype == TokenType::STAR) || (optype == TokenType::SLASH) ||
      (optype == TokenType::PERCENT)) {
    return binexpr.evalty = commonty;
  } else {
    return binexpr.evalty = BuiltInType::getInt32Ty();
  }
}

const Type* TypeResolver::operator()(Int32Exp& int32) {
  return int32.evalty = BuiltInType::getInt32Ty();
}

const Type* TypeResolver::operator()(Int64Exp& int64) {
  return int64.evalty = BuiltInType::getInt64Ty();
}

const Type* TypeResolver::operator()(const StringExp&) {
  assert(0);
}
 
const Type* TypeResolver::operator()(CastExpr& cast) {
  cast.exprty = typecheck(cast.expr.get());
  cast.evalty = getTypeFromToken(cast.type);
  return cast.evalty;
}

const Type* TypeResolver::operator()(UnaryExpr& expr) {
  const Type* exprty = typecheck(expr.right.get());
  if (expr.op.type == TokenType::BANG) {
    // !expr evaluates to integer.
    expr.evalty = BuiltInType::getInt32Ty();
  } else {
    expr.evalty = exprty;
  }

  return expr.evalty;
}

const Type* TypeResolver::operator()(Variable& var) {
  const std::string& varstr = var.sym->getName();
  auto resolvedRef = symtab_.find(varstr);
  const Type* ty = (resolvedRef != symtab_.end()) ? resolvedRef->second->getType() : nullptr;

  if (ty != nullptr) {
    return ty;
  } else if (auto fn = var.sym->getFunction()) {
    // Note: calling gettype again on function, will trigger type recomputation.
    ty = fn->sym->getType();
  } else if (auto param = var.sym->getFunctionParam()) {
    ty = param->sym->getType();
  } else {
    ty = BuiltInType::getInt32Ty();
  }

  return var.evalty = ty;
}

const Type* TypeResolver::operator()(Call& call) {
  const Function* fn = call.fn;
  if (fn->params.size() != call.args.size()) {
    errorHandler_.add(fn->name.line, " at '" + fn->name.toString() + "'",
                      "Call and function args don't match.");
  }

  const FunctionType* fnty =
    static_cast<const FunctionType*>(fn->sym->getType());
  const std::vector<const Type*>& paramsty = fnty->getParamTy();
  for (size_t i = 0; i < call.args.size(); ++i) {
    const Type* declty = paramsty[i];
    const Type* evalty = typecheck(call.args[i].get());
    const Type* promotety = sGetCommonType(declty, evalty);
    call.args[i] = sConvertTo(std::move(call.args[i]), evalty, promotety);
  }

  // result of call expression is the same as return type of function.
  call.evalty = fnty->getReturnTy();
  return call.evalty;
}
