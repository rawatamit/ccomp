#include "TackyGen.h"
#include "Token.h"
#include "ast/Tacky.h"
#include "Util.h"
#include <cassert>
#include <format>
#include <memory>
#include <vector>

using namespace ccomp;

TackyGen::TackyGen(const std::vector<std::unique_ptr<Stmt>>& stmts,
                   SymTabT& symtab, ErrorHandler& errorHandler) :
  stmts_(stmts), symtab_(symtab), errorHandler_(errorHandler)
{}

std::shared_ptr<Tacky> TackyGen::gen() {
  std::vector<std::shared_ptr<Tacky>> fns;
  for (auto& stmt : stmts_) {
    auto fn = std::visit(*this, *stmt);
    // function definitions are used
    if (fn) {
      fns.emplace_back(fn);
    }
  }

  std::vector<std::shared_ptr<Tacky>> defs;
  for (auto entry : symtab_) {
    auto sym = entry.second;
    const SymbolAttrs* attrs = sym->getAttrs();
    if (attrs->getAttributeType() == SymbolAttrs::STATIC_ATTR) {
      const InitialValue& initValue = attrs->getInitValue();
      if (initValue.getType() == InitialValue::INITIAL_INT32_VALUE ||
          initValue.getType() == InitialValue::INITIAL_LONG_VALUE) {
        defs.emplace_back(make_tacky<TackyStaticVar>(
            attrs->isGlobal(), sym->getName(), initValue));
      } else if (initValue.getType() == InitialValue::TENTATIVE_VALUE) {
        defs.emplace_back(make_tacky<TackyStaticVar>(
            attrs->isGlobal(), sym->getName(), InitialValue()));
      }
    }
  }

  return make_tacky<TackyProgram>(fns, defs);
}

std::shared_ptr<Tacky> TackyGen::gen(Expr* expr) {
  return std::visit(*this, *expr);
}

std::shared_ptr<Tacky> TackyGen::gen(Stmt* stmt) {
  return std::visit(*this, *stmt);
}

void TackyGen::gen(const std::vector<std::unique_ptr<Stmt>>& stmts) {
  for (auto& stmt : stmts) {
    gen(stmt.get());
  }
}

std::shared_ptr<Tacky> TackyGen::make_tacky_var(const Type* ty) {
  static int nextId = 0;
  std::string name = std::format("make_tacky_var.{}.tmp.{}", nextId, nextId++);
  return add_to_symtab(name, ty);
}

std::shared_ptr<Tacky> TackyGen::add_to_symtab(const std::string& name, const Type* ty) {
  auto sym = std::make_shared<Symbol>(name, false, nullptr);
  sym->setType(ty);
  sym->setAttrs(std::make_unique<SymbolAttrs>());
  symtab_.emplace(name, sym);
  return make_tacky<TackyVar>(name);
}

std::string TackyGen::unique_label(const std::string& desc) {
  static int nextId = 0;
  return std::format("T{}.{}", desc, nextId++);
}

std::string TackyGen::break_label(int loop_label) {
  return std::format("break_loop{}", loop_label);
}

std::string TackyGen::continue_label(int loop_label) {
  return std::format("continue_loop{}", loop_label);
}

std::shared_ptr<Tacky> TackyGen::operator()(const Block& stmt) {
  gen(stmt.stmts);
  return nullptr;
}

std::shared_ptr<Tacky> TackyGen::operator()(const Expression& stmt) {
  return gen(stmt.expr.get());
}

std::shared_ptr<Tacky> TackyGen::operator()(const Function& fn) {
  // generate instructions for function definition, declarations don't
  // correspond to instructions.
  if (fn.body) {
    // start with empty instructions
    instructions_.clear();
    std::vector<std::shared_ptr<Tacky>> params;
    for (auto& param: fn.params) {
      params.emplace_back(gen(param.get()));
    }

    gen(fn.body.get());

    // return 0 statement added to every function
    instructions_.emplace_back(
        make_tacky<TackyReturn>(make_tacky<TackyConstInt32>(0)));
    bool isGlobal = fn.sym->getAttrs()->isGlobal();
    return make_tacky<TackyFunction>(isGlobal, fn.sym->getName(),
                                     std::move(params),
                                     std::move(instructions_));
  }

  return nullptr;
}

std::shared_ptr<Tacky> TackyGen::operator()(const FunctionParam& param) {
  return add_to_symtab(param.sym->getName(), param.sym->getType());
}

std::shared_ptr<Tacky> TackyGen::operator()(const If& ifstmt) {
  // <instructions for condition>
  // c = <result of condition>
  auto condvar = gen(ifstmt.condition.get());
  auto end_label = make_tacky<TackyLabel>(unique_label("if_end"));

  if (ifstmt.elseBranch == nullptr) {
    // JumpIfZero(c, end)
    instructions_.emplace_back(
      make_tacky<TackyJumpIfZero>(condvar, end_label));

    // <instructions for statement>
    gen(ifstmt.thenBranch.get());
  } else {
    // JumpIfZero(c, else_label)
    auto else_label = make_tacky<TackyLabel>(unique_label("if_else"));
    instructions_.emplace_back(
      make_tacky<TackyJumpIfZero>(condvar, else_label));

    // <instructions for statement1>
    gen(ifstmt.thenBranch.get());

    // Jump(end)
    instructions_.emplace_back(make_tacky<TackyJump>(end_label));

    // Label(else_label)
    instructions_.emplace_back(else_label);

    // <instructions for statement2>
    gen(ifstmt.elseBranch.get());
  }

  // Label(end)
  instructions_.emplace_back(end_label);
  return nullptr;
}

std::shared_ptr<Tacky> TackyGen::operator()(const Return& ret) {
  // convert constant or var to a return expression
  instructions_.emplace_back(make_tacky<TackyReturn>(gen(ret.value.get())));
  return nullptr;
}

std::shared_ptr<Tacky> TackyGen::operator()(const DoWhile& loop) {
  // Label(start)
  auto loop_begin = make_tacky<TackyLabel>(unique_label("do_while"));
  instructions_.emplace_back(loop_begin);

  // <instructions for body>
  gen(loop.body.get());

  // Label(continue_label)
  instructions_.emplace_back(
    make_tacky<TackyLabel>(continue_label(loop.loop_label)));

  // <instructions for condition>
  // v = <result of condition>
  auto res = gen(loop.condition.get());

  // JumpIfNotZero(v, start)
  instructions_.emplace_back(make_tacky<TackyJumpIfNotZero>(res, loop_begin));

  // Label(break_label)
  instructions_.emplace_back(
    make_tacky<TackyLabel>(break_label(loop.loop_label)));
  return nullptr;
}

std::shared_ptr<Tacky> TackyGen::operator()(const While& loop) {
  // Label(start|continue_label)
  auto loop_begin =
    make_tacky<TackyLabel>(continue_label(loop.loop_label));
  instructions_.emplace_back(loop_begin);

  // <instructions for condition>
  // v = <result of condition>
  auto res = gen(loop.condition.get());

  // JumpIfZero(v, end|break)
  auto end_label =
    make_tacky<TackyLabel>(break_label(loop.loop_label));
  instructions_.emplace_back(make_tacky<TackyJumpIfZero>(res, end_label));

  // <instructions for body>
  gen(loop.body.get());

  // Jump(continue_label)
  instructions_.emplace_back(make_tacky<TackyJump>(loop_begin));

  // Label(break_label|end_label)
  instructions_.emplace_back(end_label);
  return nullptr;
}

std::shared_ptr<Tacky> TackyGen::operator()(const For& loop) {
  // <instructions for init>
  if (loop.init) {
    gen(loop.init.get());
  }

  // Label(start)
  auto loop_begin = make_tacky<TackyLabel>(unique_label("forloop"));
  instructions_.emplace_back(loop_begin);

  auto end_label =
    make_tacky<TackyLabel>(break_label(loop.loop_label));

  // <instructions for condition>
  // v = <result of condition>
  if (loop.condition) {
    auto res = gen(loop.condition.get());

    // JumpIfZero(v, end|break)
    instructions_.emplace_back(make_tacky<TackyJumpIfZero>(res, end_label));
  }

  // <instructions for body>
  gen(loop.body.get());

  // Label(continue_label)
  auto cont_label =
    make_tacky<TackyLabel>(continue_label(loop.loop_label));
  instructions_.emplace_back(cont_label);

  // <instructions for post>
  if (loop.post) {
    gen(loop.post.get());
  }

  // Jump(start)
  instructions_.emplace_back(make_tacky<TackyJump>(loop_begin));

  // Label(end|break_label)
  instructions_.emplace_back(end_label);
  return nullptr;
}

std::shared_ptr<Tacky> TackyGen::operator()(const Decl& decl) {
  // file scope, extern, and static declarations are handled by symtab
  // iteration.
  if (decl.fileScope || ((decl.storage == Scope::STORAGE_STATIC) ||
                         (decl.storage == Scope::STORAGE_EXTERN))) {
    return nullptr;
  }

  if (auto& init = decl.init) {
    // lvalue is Var(v)
    auto dst = gen(decl.name.get());
    auto src = gen(init.get());

    // copy src to dst
    instructions_.emplace_back(make_tacky<TackyCopy>(src, dst));
  }

  // declaration is a statement
  return nullptr;
}

std::shared_ptr<Tacky> TackyGen::operator()(const Assign& expr) {
  // lvalue is Var(v)
  auto dst = gen(expr.lvalue.get());
  auto src = gen(expr.value.get());

  // copy src to dst
  instructions_.emplace_back(make_tacky<TackyCopy>(src, dst));
  return dst;
}

std::shared_ptr<Tacky> TackyGen::operator()(const Null&) {
  return nullptr;
}

std::shared_ptr<Tacky> TackyGen::operator()(const Break& flow) {
  instructions_.emplace_back(make_tacky<TackyJump>(
    make_tacky<TackyLabel>(break_label(flow.loop_label))));
  return nullptr;
}

std::shared_ptr<Tacky> TackyGen::operator()(const Continue& flow) {
  instructions_.emplace_back(make_tacky<TackyJump>(
    make_tacky<TackyLabel>(continue_label(flow.loop_label))));
  return nullptr;
}

std::shared_ptr<Tacky> TackyGen::operator()(const Conditional& ternary) {
  // <instructions for condition>
  // c = <result of condition>
  auto condvar = gen(ternary.condition.get());

  // JumpIfZero(c, e2_label)
  auto else_label = make_tacky<TackyLabel>(unique_label("ternary_else"));
  instructions_.emplace_back(make_tacky<TackyJumpIfZero>(condvar, else_label));

  // <instructions to calculate e1>
  // v1 = <result of e1>
  // result = v1
  auto thenRes = gen(ternary.thenExp.get());
  auto result = make_tacky_var(ternary.evalty);
  instructions_.emplace_back(make_tacky<TackyCopy>(thenRes, result));

  // Jump(end)
  auto end_label = make_tacky<TackyLabel>(unique_label("ternary_end"));
  instructions_.emplace_back(make_tacky<TackyJump>(end_label));

  // Label(e2_label)
  instructions_.emplace_back(else_label);

  // <instructions to calculate e2>
  // v2 = <result of e2>
  // result = v2
  auto elseRes = gen(ternary.elseExp.get());
  instructions_.emplace_back(make_tacky<TackyCopy>(elseRes, result));

  // Label(end)
  instructions_.emplace_back(end_label);
  return result;
}

std::shared_ptr<Tacky> TackyGen::genLogical(const BinaryExpr& expr) {
  TokenType op = expr.op.type;
  // <instructions for e1>
  // v1 = <result of e1>
  auto v1 = gen(expr.left.get());

  auto result_both_check_label = make_tacky<TackyLabel>(unique_label("logical"));
  auto end_label = make_tacky<TackyLabel>(unique_label("logical"));

  // JumpIfZero|JumpIfNotZero(v1, result_both_check_label)
  if (op == TokenType::AMPERSAND_AMPERSAND) {
    instructions_.emplace_back(
      make_tacky<TackyJumpIfZero>(v1, result_both_check_label));
  } else if (op == TokenType::PIPE_PIPE) {
    instructions_.emplace_back(
      make_tacky<TackyJumpIfNotZero>(v1, result_both_check_label));
  }

  // <instructions for e2>
  // v2 = <result of e2>
  auto v2 = gen(expr.right.get());

  // JumpIfZero|JumpIfNotZero(v2, result_both_check_label)
  if (op == TokenType::AMPERSAND_AMPERSAND) {
    instructions_.emplace_back(
      make_tacky<TackyJumpIfZero>(v2, result_both_check_label));
  } else if (op == TokenType::PIPE_PIPE) {
    instructions_.emplace_back(
      make_tacky<TackyJumpIfNotZero>(v2, result_both_check_label));
  }

  // result = 1|0
  // && returns a 1 if both conditions were true. || returns a 0 if both
  // conditions are false. Note we are using JumpIfZero for && and JumpIfNotZero
  // for ||.
  int result_after_two_checks = (op == TokenType::AMPERSAND_AMPERSAND) ? 1 : 0;
  auto result = make_tacky_var(expr.evalty);
  instructions_.emplace_back(make_tacky<TackyCopy>(
    make_tacky<TackyConstInt32>(result_after_two_checks), result));

  // Jump(end)
  instructions_.emplace_back(make_tacky<TackyJump>(end_label));

  // Label(result_both_check_label)
  instructions_.emplace_back(result_both_check_label);

  // result = 0|1
  int result_after_label = 1 - result_after_two_checks;
  instructions_.emplace_back(
    make_tacky<TackyCopy>(make_tacky<TackyConstInt32>(result_after_label), result));

  // Label(end)
  instructions_.emplace_back(end_label);
  return result;
}

std::shared_ptr<Tacky> TackyGen::operator()(const BinaryExpr& expr) {
  // binary_operator = Add | Subtract | Multiply | Divide | Remainder | Equal |
  // NotEqual | LessThan | LessOrEqual | GreaterThan | GreaterOrEqual
  TokenType op = expr.op.type;
  bool isLogical = isLogicalOp(op);
  assert(one_of(op, {TokenType::PLUS, TokenType::MINUS, TokenType::STAR,
                TokenType::SLASH, TokenType::PERCENT}) ||
         isLogical || isRelationalOp(op));

  // Logical operations are short circuited.
  if (isLogical) {
    return genLogical(expr);
  }

  // v1 = emit_tacky(e1, instructions)
  auto src1 = gen(expr.left.get());

  // v2 = emit_tacky(e2, instructions)
  auto src2 = gen(expr.right.get());

  // dst_name = make_temporary()
  // dst = Var(dst_name)
  auto dst = make_tacky_var(expr.evalty);

  // tacky_op = convert_binop(op)
  // instructions.append(Binary(tacky_op, v1, v2, dst))
  // NOTE: tacky_op and expr->Operator are the same.
  instructions_.emplace_back(
    make_tacky<TackyBinary>(expr.op, src1, src2, dst));
  return dst;
}

std::shared_ptr<Tacky> TackyGen::operator()(const Int32Exp& num) {
  return make_tacky<TackyConstInt32>(num.int32);
}

std::shared_ptr<Tacky> TackyGen::operator()(const Int64Exp& num) {
  return make_tacky<TackyConstInt64>(num.int64);
}

std::shared_ptr<Tacky> TackyGen::operator()(const StringExp&) {
  assert(0);
}

std::shared_ptr<Tacky> TackyGen::operator()(const CastExpr& cexpr) {
  auto val = gen(cexpr.expr.get());
  // no promotion required
  const Type* ty = cexpr.evalty;
  if (ty == cexpr.exprty) {
    return val;
  }

  auto dst = make_tacky_var(ty);
  if (ty == BuiltInType::getInt64Ty()) {
    instructions_.emplace_back(
      make_tacky<TackySignExtend>(val, dst));
  } else {
    instructions_.emplace_back(
      make_tacky<TackyTruncate>(val, dst));
  }

  return dst;
}

std::shared_ptr<Tacky> TackyGen::operator()(const UnaryExpr& expr) {
  // unary_operator = Complement | Negate | Not
  assert(one_of(expr.op.type, {TokenType::TILDE, TokenType::MINUS,
                TokenType::BANG}));

  // src = emit_tacky(inner, instructions)
  auto src = gen(expr.right.get());

  // dst_name = make_temporary()
  // dst = Var(dst_name)
  auto dst = make_tacky_var(expr.evalty);

  // tacky_op = convert_unop(op)
  // instructions.append(Unary(tacky_op, src, dst))
  // NOTE: tacky_op and expr->Operator are the same.
  instructions_.emplace_back(
    make_tacky<TackyUnary>(expr.op, src, dst));

  return dst;
}

std::shared_ptr<Tacky> TackyGen::operator()(const Variable& var) {
  return add_to_symtab(var.sym->getName(), var.sym->getType());
}

std::shared_ptr<Tacky> TackyGen::operator()(const Call& call) {
  // TODO: only functions name can appear in a call.
  auto fn = std::get_if<Variable>(call.callee.get());
  assert(fn != nullptr);

  std::vector<std::shared_ptr<Tacky>> args;
  for (auto& arg : call.args) {
    args.emplace_back(gen(arg.get()));
  }

  auto fname = fn->name.toString();
  auto dst = make_tacky_var(call.evalty);
  instructions_.emplace_back(make_tacky<TackyFunCall>(fname, args, dst));
  return dst;
}
