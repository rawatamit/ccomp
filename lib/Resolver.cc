#include "Resolver.h"
#include "ast/Stmt.h"
#include <cassert>

using namespace ccomp;

bool isDiffLinkage(Scope::Linkage a, Scope::Linkage b) {
  return (a == ccomp::Scope::LINKAGE_INTERNAL && b == ccomp::Scope::LINKAGE_EXTERNAL) ||
         (a == ccomp::Scope::LINKAGE_EXTERNAL && b == ccomp::Scope::LINKAGE_INTERNAL);
}

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

void Resolver::resolveFunction(Function& fn,
                               FunctionType type) {
  FunctionType enclosingFn = currentFunction_;
  currentFunction_ = type;
  beginScope<FunctionScope>();
  fn.scope = std::make_shared<Scope>(*curScope_);
  for (auto& param : fn.params) {
    resolve(param.get());
  }

  if (fn.body) {
    resolve(fn.body.get());
  }
  endScope();
  currentFunction_ = enclosingFn;
}

template<typename T>
void Resolver::beginScope(bool parent) {
  curScope_ = std::make_shared<T>(parent ? curScope_: nullptr);
}

void Resolver::endScope() {
  curScope_ = curScope_->getEnclosingScope();
}

void Resolver::declare(const Token& name, Function* fn) {
  Scope::ScopeDetail detail = curScope_->resolve(name);
  bool foundVar = (detail.scope != nullptr);
  bool isSameScope = foundVar && (curScope_->getLevel() == detail.scope->getLevel());
  if (isSameScope
      && isDiffLinkage(detail.linkage, Scope::LINKAGE_EXTERNAL)) {
      errorHandler_.add(
          name.line, " at '" + name.lexeme + "'",
          "Redefining variable as function.");
  }

  auto pfn = getFunction(detail);
  // Function with the same name.
  // Number of argument must match.
  if (pfn == nullptr) {
    curScope_->declare(name, fn, Scope::LINKAGE_EXTERNAL);
  } else if ((pfn->body != nullptr) && (fn->body != nullptr)) {
    errorHandler_.add(
        name.line, " at '" + name.lexeme + "'",
        "Function definition repeated.");
  } else if (fn->body != nullptr) {
    // replace declaration with definition
    curScope_->declare(name, fn, Scope::LINKAGE_EXTERNAL);
  }
}

void Resolver::declare(const Token &name, Variable* var) {
  Scope::ScopeDetail detail = curScope_->resolve(name);
  bool foundVar = (detail.scope != nullptr);
  bool isSameScope = foundVar && (curScope_->getLevel() == detail.scope->getLevel());
  if (isSameScope
      && isDiffLinkage(detail.linkage, Scope::LINKAGE_INTERNAL)) {
      errorHandler_.add(
          name.line, " at '" + name.lexeme + "'",
          "Redefining function as variable.");
  } else if (isSameScope && (detail.value.type() == typeid(Variable*))) {
    errorHandler_.add(
        name.line, " at '" + name.lexeme + "'",
        "Variable with this name already declared in this scope.");
  } else if (detail.value.type() == typeid(FunctionParam*)) {
    // Function body and parameters share the same scope.
    // Even if in code they don't share the same scope, if a var with
    // same name is resolved as a function parameter.
    errorHandler_.add(
        name.line, " at '" + name.lexeme + "'",
        "Redefining function parameter.");
  }

  curScope_->declare(name, var, Scope::LINKAGE_INTERNAL);
}

void Resolver::declare(const Token& name, FunctionParam* param) {
  Scope::ScopeDetail detail = curScope_->resolve(name);
  if (((detail.value.type() == typeid(Variable*)) ||
      (detail.value.type() == typeid(FunctionParam*))) &&
      (curScope_->getLevel() == detail.scope->getLevel())) {
    errorHandler_.add(
        name.line, " at '" + name.lexeme + "'",
        "Variable with this name already declared in this scope.");
  }
  curScope_->declare(name, param, Scope::LINKAGE_INTERNAL);
}

bool Resolver::isFunctionDefined(const Token& tok) {
  auto detail = curScope_->resolve(tok);
  return getFunction(detail) != nullptr;
}

bool Resolver::isVariableDefined(const Token& tok) {
  auto detail = curScope_->resolve(tok);
  return isVariable(detail);
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

void Resolver::operator()(Function& fn) {
  // Function definition appears inside another function.
  // Note that function declaration can appear inside another function.
  Scope::ScopeDetail detail = curScope_->resolve(fn.name);
  auto pfn = getFunction(detail);
  if ((pfn && pfn->body != nullptr) && (fn.body != nullptr)) {
    errorHandler_.add(fn.name.line, " at function " + fn.name.toString(),
                     "Function already defined.");
  } else if ((fn.body != nullptr) && (curScope_->getLevel() > 0)) {
    errorHandler_.add(fn.name.line, " at function " + fn.name.toString(),
                      "Function can only be defined at top-level.");
  } else {
    declare(fn.name, &fn);
    resolveFunction(fn, FUNCTION);
  }
}

void Resolver::operator()(FunctionParam& param) {
  declare(param.name, &param);
  auto detail = curScope_->resolve(param.name);
  if (detail.scope != nullptr) {
    // scope level is used for uniquifying variable names in TackyGen.
    param.level = detail.scope->getLevel();
  }
}

void Resolver::operator()(const If& ifstmt) {
  resolve(ifstmt.condition.get());
  resolve(ifstmt.thenBranch.get());
  if (auto& elseBranch = ifstmt.elseBranch) {
    resolve(elseBranch.get());
  }
}

void Resolver::operator()(const Block& block) {
  beginScope<LocalScope>();
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

  if (ret.value) {
    resolve(ret.value.get());
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
  beginScope<LocalScope>();
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
  declare(var->name, var);
  // Resolve immediately and store reference.
  var->level = curScope_->getLevel();
  var->var = curScope_->resolve(var->name);

  if (decl.init != nullptr) {
    resolve(decl.init.get());
  }
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

  auto detail = curScope_->resolve(var.name);
  if (detail.scope != nullptr) {
    // scope level is used for uniquifying variable names in TackyGen.
    var.level = detail.scope->getLevel();
    // var is reference to resolved variable.
    var.var = detail.value;
  } else {
    const auto& id = var.name.toString();
    errorHandler_.add(var.name.line, " at '" + id + "'",
                      "Variable not defined before use.");
  }
}

void Resolver::operator()(Call& call) {
  auto callee = std::get_if<Variable>(call.callee.get());
  if (callee == nullptr) {
    errorHandler_.add(0, std::string(" at '") + "name" + "'",
                      "Invalid target for function call.");
  }

  auto detail = curScope_->resolve(callee->name);
  Function* fn = getFunction(detail);
  if (fn != nullptr) {
    call.fn = fn;
    for (auto& arg : call.args) {
      resolve(arg.get());
    }
  } else {
    errorHandler_.add(callee->name.line, " at '" + callee->name.toString() + "'",
                      "Function not defined before use.");
  }
}
