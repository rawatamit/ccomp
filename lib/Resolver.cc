#include "Resolver.h"
#include <cassert>

using namespace ccomp;

void Resolver::resolve(const std::vector<std::unique_ptr<Stmt>>& prog) {
  for (auto& stmt : prog) {
    resolve(stmt.get());
  }
}

void Resolver::resolve(Stmt* stmt) {
  std::visit(*this, *stmt);
}

void Resolver::resolve(Expr* expr) {
  std::visit(*this, *expr);
}

int Resolver::resolveLocal(const Token &tok) {
  auto scopes_rend = scopes_.rend();
  for (auto it = scopes_.rbegin(); it != scopes_rend; ++it) {
    if (it->find(tok.lexeme) != it->end()) {
      return std::distance(it, scopes_rend);
    }
  }

  // TODO: not found, assume global?
  return -1;
}

void Resolver::resolveFunction(const Function& fn,
                               FunctionType type) {
  FunctionType enclosingFn = currentFunction_;
  currentFunction_ = type;
  beginScope();
  for (auto &param : fn.params) {
    declare(param);
    define(param);
  }
  resolve(fn.body);
  endScope();
  currentFunction_ = enclosingFn;
}

void Resolver::beginScope() {
  scopes_.push_back(std::unordered_map<std::string, bool>());
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

void Resolver::beginLoop(int* label) {
  nested_loop_labels_.push_back(loop_label_++);
  copyLoopLabel(label);
}

void Resolver::endLoop() {
  nested_loop_labels_.pop_back();
}

void Resolver::copyLoopLabel(int* label) {
  *label = nested_loop_labels_.back();
}

#if 0
void
Resolver::visitClass(std::unique_ptr<Class> klass) {
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

void Resolver::operator()(const Function& fn) {
  declare(fn.name);
  define(fn.name);
  resolveFunction(fn, FUNCTION);
}

void Resolver::operator()(const If& ifstmt) {
  resolve(ifstmt.condition.get());
  resolve(ifstmt.thenBranch.get());
  if (auto& elseBranch = ifstmt.elseBranch) {
    resolve(elseBranch.get());
  }
}

void Resolver::operator()(const Block& block) {
  beginScope();
  resolve(block.stmts);
  endScope();
}

void Resolver::operator()(const Expression& stmt) {
  resolve(stmt.expr.get());
}

void Resolver::operator()(const Return& ret) {
  if (currentFunction_ == NONEF) {
    errorHandler_.add(ret.keyword.line, " at 'return'",
                     "Cannot return from top-level code.");
  }

  if (auto& retVal = ret.value) {
    if (currentFunction_ == INITIALIZER) {
      errorHandler_.add(ret.keyword.line, " at 'return'",
                       "Cannot return a value from an initialiser.");
    } else {
      resolve(retVal.get());
    }
  }
}

void Resolver::operator()(DoWhile& loop) {
  beginLoop(&loop.loop_label);
  resolve(loop.body.get());
  resolve(loop.condition.get());
  endLoop();
}

void Resolver::operator()(While& loop) {
  beginLoop(&loop.loop_label);
  resolve(loop.condition.get());
  resolve(loop.body.get());
  endLoop();
}

void Resolver::operator()(For& loop) {
  beginLoop(&loop.loop_label);
  beginScope();
  if (loop.init) {
    resolve(loop.init.get());
  }

  if (loop.condition) {
    resolve(loop.condition.get());
  }

  if (loop.post) {
    resolve(loop.post.get());
  }

  resolve(loop.body.get());
  endScope();
  endLoop();
}

void Resolver::operator()(const Decl& decl) {
  auto var = std::get_if<Variable>(decl.name.get());
  assert(var != nullptr);
  // Declare variable name, and set scope level on variable.
  declare(var->name);
  var->level = scopes_.size();

  if (decl.init != nullptr) {
    resolve(decl.init.get());
  }
  define(var->name);
}

void Resolver::operator()(const Null&)
{}

void Resolver::operator()(Break& flow) {
  if (!nested_loop_labels_.empty()) {
    copyLoopLabel(&flow.loop_label);
  } else {
    errorHandler_.add(flow.loc.line, " at 'break'",
                      "break must be inside a loop or switch.");
  }
}

void Resolver::operator()(Continue& flow) {
  if (!nested_loop_labels_.empty()) {
    copyLoopLabel(&flow.loop_label);
  } else {
    errorHandler_.add(flow.loc.line, " at 'continue'",
                      "break must be inside a loop or switch.");
  }
}

void Resolver::operator()(const Assign& assign) {
  auto& lvalue = assign.lvalue;
  resolve(lvalue.get());
  if (std::holds_alternative<Variable>(*lvalue)) {
    resolve(assign.value.get());
  } else {
    // TODO: fix line number.
    errorHandler_.add(0, " at assign ",
                      "Invalid target for assignment.");
  }
}

void Resolver::operator()(const Conditional& tertiary) {
  resolve(tertiary.condition.get());
  resolve(tertiary.thenExp.get());
  resolve(tertiary.elseExp.get());
}

void Resolver::operator()(const BinaryExpr& binexpr) {
  resolve(binexpr.left.get());
  resolve(binexpr.right.get());
}

void Resolver::operator()(const LiteralExpr&) {
}

void Resolver::operator()(const UnaryExpr& unary) {
  resolve(unary.right.get());
}

void Resolver::operator()(Variable& var) {
#if 0
  if (!scopes_.empty()) {
    auto it = scopes_.back().find(var->name.lexeme);
    if (it != scopes_.back().end() and it->second == false) {
      errorHandler_.add(var->name.line, " at '" + var->name.lexeme + "'",
                       "Cannot read local variable in its own initialiser.");
    }
  }
#endif

  int level = resolveLocal(var.name);
  if (level >= 0) {
    // Set the scope level at which this variable is defined.
    // This is used for uniquifying variable names in TackyGen.
    var.level = level;
  } else {
    const auto& id = var.name.toString();
    errorHandler_.add(var.name.line, " at '" + id + "'",
                      "Variable not defined before use.");
  }
}
