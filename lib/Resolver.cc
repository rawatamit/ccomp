#include "Resolver.h"

using namespace ccomp;

void Resolver::resolve(const std::vector<std::shared_ptr<Stmt>> &prog) {
  for (auto stmt : prog) {
    resolve(stmt);
  }
}

void Resolver::resolve(std::shared_ptr<Stmt> stmt) {
  stmt->accept(*this);
}

void Resolver::resolve(std::shared_ptr<Expr> expr) {
  expr->accept(*this);
}

void Resolver::resolveDecls(const std::vector<std::shared_ptr<Decl>>& decls) {
  for (auto decl : decls) {
    resolve(decl);
  }
}

void Resolver::resolveLocal(std::shared_ptr<Expr> expr, const Token &tok) {
  for (int i = scopes_.size() - 1; i >= 0; --i) {
    if (scopes_[i].find(tok.lexeme) != scopes_[i].end()) {
      return;
    }
  }

  // not found, assume global
}

void Resolver::resolveFunction(std::shared_ptr<Function> fn,
                               FunctionType type) {
  FunctionType enclosingFn = currentFunction_;
  currentFunction_ = type;
  beginScope();
  for (auto &param : fn->params) {
    declare(param);
    define(param);
  }
  resolve(fn->body);
  endScope();
  currentFunction_ = enclosingFn;
}

void Resolver::beginScope() {
  scopes_.push_back(std::map<std::string, bool>());
}

void Resolver::endScope() {
  scopes_.pop_back();
}

void Resolver::declare(const Token &name) {
  if (!scopes_.empty()) {
    if (scopes_.back().find(name.lexeme) != scopes_.back().end()) {
      errorHandler_.add(
          name.line, " at '" + name.lexeme + "'",
          "Variable with this name already declared in this scope.");
    }
    scopes_.back()[name.lexeme] = false;
  }
}

void Resolver::define(const Token &name) {
  if (!scopes_.empty()) {
    scopes_.back()[name.lexeme] = true;
  }
}

#if 0
std::any
Resolver::visitClass(std::shared_ptr<Class> klass) {
  ClassType enclosingClass = currentClass;
  currentClass = ClassType::CLASS;
  declare(klass->name);
  define(klass->name);

  if (klass->superclass != nullptr) {
    if (klass->superclass->name.lexeme == klass->name.lexeme) {
      errorHandler.add(klass->name.line, " at 'return'",
                       "A class cannot inherit from itself.");
    } else {
      currentClass = SUBCLASS;
      resolve(klass->superclass);
      beginScope();
      scopes.back()["super"] = true;
    }
  }

  // define this
  beginScope();
  scopes.back()["this"] = true;

  for (auto method : klass->methods) {
    FunctionType decl = (method->name.lexeme == "init") ? INITIALIZER : METHOD;
    resolveFunction(method, decl);
  }

  endScope();
  if (klass->superclass != nullptr) {
    endScope();
  }

  currentClass = enclosingClass;
  return nullptr;
}
#endif

std::any Resolver::visitFunction(std::shared_ptr<Function> fn) {
  declare(fn->name);
  define(fn->name);
  resolveFunction(fn, FUNCTION);
  return nullptr;
}

std::any Resolver::visitExpression(std::shared_ptr<Expression> stmt) {
  resolve(stmt->expr);
  return nullptr;
}

std::any Resolver::visitReturn(std::shared_ptr<Return> ret) {
  if (currentFunction_ == NONEF) {
    errorHandler_.add(ret->keyword.line, " at 'return'",
                     "Cannot return from top-level code.");
  }

  if (ret->value != nullptr) {
    if (currentFunction_ == INITIALIZER) {
      errorHandler_.add(ret->keyword.line, " at 'return'",
                       "Cannot return a value from an initialiser.");
    } else {
      resolve(ret->value);
    }
  }

  return nullptr;
}

std::any Resolver::visitDecl(std::shared_ptr<Decl> decl) {
  declare(decl->name);
  if (decl->init != nullptr) {
    resolve(decl->init);
  }
  define(decl->name);
  return nullptr;
}

std::any Resolver::visitBlock(std::shared_ptr<Block> block) {
  beginScope();
  resolve(block->stmts);
  endScope();
  return nullptr;
}

std::any Resolver::visitAssign(std::shared_ptr<Assign> assign) {
  auto lvalue = assign->lvalue;
  resolve(lvalue);
  if (auto e = std::dynamic_pointer_cast<Variable>(lvalue)) {
    resolve(assign->value);
    //resolveLocal(expr, expr->name);
  } else {
    // TODO: fix line number.
    errorHandler_.add(0, " at assign ",
                      "Invalid target for assignment.");
  }
  return nullptr;
}

std::any Resolver::visitBinaryExpr(std::shared_ptr<BinaryExpr> binexpr) {
  resolve(binexpr->left);
  resolve(binexpr->right);
  return nullptr;
}

std::any Resolver::visitLiteralExpr(std::shared_ptr<LiteralExpr>) {
  return nullptr;
}

std::any Resolver::visitUnaryExpr(std::shared_ptr<UnaryExpr> unary) {
  resolve(unary->right);
  return nullptr;
}

std::any Resolver::visitVariable(std::shared_ptr<Variable> var) {
#if 0
  if (!scopes_.empty()) {
    auto it = scopes_.back().find(var->name.lexeme);
    if (it != scopes_.back().end() and it->second == false) {
      errorHandler_.add(var->name.line, " at '" + var->name.lexeme + "'",
                       "Cannot read local variable in its own initialiser.");
    }
  }
#endif

  auto it = scopes_.back().find(var->name.lexeme);
  if (it == scopes_.back().end()) {
    errorHandler_.add(var->name.line, " at '" + var->name.lexeme + "'",
                      "Variable not defined before use.");
  }

  resolveLocal(var, var->name);
  return nullptr;
}