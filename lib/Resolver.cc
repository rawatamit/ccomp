#include "Resolver.h"
#include "Scope.h"
#include "Symbol.h"
#include "Util.h"
#include "ast/Stmt.h"
#include <format>
#include <cassert>

using namespace ccomp;

int Resolver::uniqueId_ = 0;

Resolver::Resolver(ErrorHandler& errorHandler)
  : errorHandler_(errorHandler),
    currentFunction_(NONEF),
    globalScope_(std::make_shared<Scope>()),
    curScope_(globalScope_),
    loop_label_(0)
{}

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
  const Type* enclosingRetTy = currentFunctionReturnTy_;
  currentFunction_ = type;
  currentFunctionReturnTy_ = getTypeFromToken(fn.returnty);
  beginScope();
  for (auto& param : fn.params) {
    resolve(param.get());
  }

  if (fn.body) {
    resolve(fn.body.get());
  }

  endScope();
  currentFunction_ = enclosingFn;
  currentFunctionReturnTy_ = enclosingRetTy;
}

void Resolver::beginScope() {
  curScope_ = std::make_shared<Scope>(curScope_);
}

void Resolver::endScope() {
  curScope_ = curScope_->getEnclosingScope();
}

void Resolver::declare(const Token& name, Function* fn) {
  auto sym = curScope_->resolve(name);
  int level = sym ? sym->getNestingLevel() : -1;
  bool isSameScope = (curScope_->getNestingLevel() == level);
  if (isSameScope && !sym->hasExternalLinkage()) {
      errorHandler_.add(
          name.line, " at '" + name.lexeme + "'",
          "Redefining variable as function.");
  }

  auto pfn = sym ? sym->getFunction() : nullptr;
  if (pfn == nullptr) {
    sym = curScope_->declare(name, getUniqueName(*fn), true, fn, curScope_);
  } else if ((pfn->body != nullptr) && (fn->body != nullptr)) {
    errorHandler_.add(
        name.line, " at '" + name.lexeme + "'",
        "Function definition repeated.");
  } else if (fn->body != nullptr) {
    // replace declaration with definition
    sym = curScope_->declare(name, getUniqueName(*fn), true, fn, curScope_);
  }

  fn->sym = sym;
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

std::string Resolver::getUniqueName(const Decl& decl, bool hasExternalLinkage) {
  auto var = std::get_if<Variable>(decl.name.get());
  if (hasExternalLinkage) {
    return var->name.toString();
  } else {
    return std::format("{}_{}_{}", var->name.toString(), curScope_->getNestingLevel(),
                       uniqueId_++);
  }
}

std::string Resolver::getUniqueName(const Function& fn) {
  return fn.name.toString();
}

std::string Resolver::getUniqueName(const FunctionParam& param) {
  return std::format("{}_{}_{}", param.name.toString(), curScope_->getNestingLevel(),
                     uniqueId_++);
}

void Resolver::operator()(Function& fn) {
  // Function definition appears inside another function.
  // Note that function declaration can appear inside another function.
  if ((fn.body != nullptr) && !fn.fileScope) {
    errorHandler_.add(fn.name.line, " at function " + fn.name.toString(),
                      "Function can only be defined at top-level.");
  } else {
    auto sym = curScope_->resolve(fn.name);
    auto pfn = sym ? sym->getFunction() : nullptr;
    if ((pfn && pfn->body != nullptr) && (fn.body != nullptr)) {
      errorHandler_.add(fn.name.line, " at function " + fn.name.toString(),
                      "Function already defined.");
    } else {
      declare(fn.name, &fn);
      resolveFunction(fn, FUNCTION);
    }
  }
}

void Resolver::operator()(FunctionParam& param) {
  auto name = param.name;
  auto sym = curScope_->resolve(name);
  if (sym && (sym->isVariable() || sym->isFunctionParam()) &&
      (curScope_->getNestingLevel() == sym->getNestingLevel())) {
    errorHandler_.add(
        name.line, " at '" + name.lexeme + "'",
        "Variable with this name already declared in this scope.");
  }

  sym =
    curScope_->declare(name, getUniqueName(param), false, &param, curScope_);
  param.sym = sym;
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

void Resolver::operator()(Return& ret) {
  if (currentFunction_ == NONEF) {
    errorHandler_.add(ret.keyword.line, " at 'return'",
                     "Cannot return from top-level code.");
  }

  ret.fnReturnTy = currentFunctionReturnTy_;
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

  if (decl.fileScope) {
    var->sym = globalScope_->declare(var->name, getUniqueName(decl, true), true,
                                     var, curScope_);
  } else {
    // Declare variable name, and set scope level on variable.
    Token name = var->name;
    auto oldsym = curScope_->resolve(name);

    int level = oldsym ? oldsym->getNestingLevel() : -1;
    bool isSameScope = (curScope_->getNestingLevel() == level);
    bool isFunctionParam = (oldsym && oldsym->isFunctionParam());
    bool declHasExternalLinkage = (decl.storage == Scope::STORAGE_EXTERN);

    // found another declaration in the same scope or a function parameter with
    // a different linkage than this declaration.
    if ((isFunctionParam || isSameScope) && !(oldsym->hasExternalLinkage() &&
        declHasExternalLinkage)) {
        errorHandler_.add(
            name.line, " at '" + name.lexeme + "'",
            "Conflicting definitions of variable with different linkages.");
    }

    if (declHasExternalLinkage) {
      var->sym = curScope_->declare(name, getUniqueName(decl, true), true, var,
                                    curScope_);
      return;
    }

    if (isSameScope && oldsym->hasExternalLinkage()) {
        errorHandler_.add(
            name.line, " at '" + name.lexeme + "'",
            "Redefining function as variable.");
    } else if (isSameScope && oldsym->isVariable()) {
      errorHandler_.add(
          name.line, " at '" + name.lexeme + "'",
          "Variable with this name already declared in this scope.");
    } else if (oldsym && oldsym->isFunctionParam()) {
      // Function body and parameters share the same scope.
      // Even if in code they don't share the same scope, if a var with
      // same name is resolved as a function parameter.
      errorHandler_.add(
          name.line, " at '" + name.lexeme + "'",
          "Redefining function parameter.");
    }

    // Store resolved reference.
    var->sym = curScope_->declare(name, getUniqueName(decl, false), false, var,
                                  curScope_);

    if (decl.init != nullptr) {
      resolve(decl.init.get());
    }
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
  if (std::get_if<Variable>(lvalue.get())) {
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

void Resolver::operator()(const Int32Exp&) {
}

void Resolver::operator()(const Int64Exp&) {
}

void Resolver::operator()(const StringExp&) {
}

void Resolver::operator()(const CastExpr& cast) {
  resolve(cast.expr.get());
}

void Resolver::operator()(const UnaryExpr& unary) {
  resolve(unary.right.get());
}

void Resolver::operator()(Variable& var) {
  auto sym = curScope_->resolve(var.name);
  if (sym) {
    // sym is reference to resolved variable.
    var.sym = sym;
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
  const Function* fn = detail->getFunction();
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
