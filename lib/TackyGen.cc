#include "TackyGen.h"
#include "Token.h"
//#include "ast/Asm.h"
#include "ast/Tacky.h"
#include "Util.h"
#include <cassert>
#include <format>
#include <memory>
#include <vector>

using namespace ccomp;

TackyGen::TackyGen(const std::vector<std::unique_ptr<Stmt>>& stmts, ErrorHandler &errorHandler) :
  stmts_(stmts), errorHandler_(errorHandler)
{}

std::shared_ptr<Tacky> TackyGen::gen() {
  std::vector<std::shared_ptr<Tacky>> fns;
  for (auto& stmt : stmts_) {
    auto fn = std::visit(*this, *stmt);
    fns.emplace_back(fn);
  }
  return make_tacky<TackyProgram>(fns);
}

std::shared_ptr<Tacky> TackyGen::gen(Expr* expr) {
  return std::visit(*this, *expr);
}

std::shared_ptr<Tacky> TackyGen::gen(Stmt* stmt) {
  return std::visit(*this, *stmt);
}

void TackyGen::gen(const std::vector<std::unique_ptr<Stmt>>& stmts) {
  std::vector<std::shared_ptr<Tacky>> genstmts;
  for (auto& stmt : stmts) {
    gen(stmt.get());
  }
}

std::string TackyGen::unique_var() {
  static int nextId = 0;
  return std::format("tmp.{}", nextId++);
}

std::string TackyGen::unique_label(const std::string& desc) {
  static int nextId = 0;
  return std::format("T{}.{}", desc, nextId++);
}

std::shared_ptr<Tacky> TackyGen::operator()(const Block& stmt) {
  gen(stmt.stmts);
  return nullptr;
}

std::shared_ptr<Tacky> TackyGen::operator()(const Expression& stmt) {
  return gen(stmt.expr.get());
}

std::shared_ptr<Tacky> TackyGen::operator()(const Function& fn) {
  // start with empty instructions
  instructions_.clear();
  gen(fn.body);

  // return 0 statement added to every function

  instructions_.emplace_back(
    make_tacky<TackyReturn>(make_tacky<TackyConstant>(0)));
  return make_tacky<TackyFunction>(fn.name, std::move(instructions_));
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

std::shared_ptr<Tacky> TackyGen::operator()(const While& Stmt) {
  gen(Stmt.condition.get());
  gen(Stmt.body.get());
  return nullptr;
}

std::shared_ptr<Tacky> TackyGen::operator()(const Decl& decl) {
  if (auto& init = decl.init) {
    // lvalue is Var(v)
    auto dst = gen(decl.name.get());
    auto src = gen(init.get());

    // copy src to dst
    instructions_.emplace_back(
      make_tacky<TackyCopy>(src, dst));
    return make_tacky<TackyCopy>(src, dst);
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
  auto result = make_tacky<TackyVar>(unique_var());
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
  TokenType op = expr.Operator.type;
  // <instructions for e1>
  // v1 = <result of e1>
  std::shared_ptr<Tacky> v1 = gen(expr.left.get());

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
  std::shared_ptr<Tacky> v2 = gen(expr.right.get());

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
  auto result = make_tacky<TackyVar>(unique_var());
  instructions_.emplace_back(make_tacky<TackyCopy>(
    make_tacky<TackyConstant>(result_after_two_checks), result));

  // Jump(end)
  instructions_.emplace_back(make_tacky<TackyJump>(end_label));

  // Label(result_both_check_label)
  instructions_.emplace_back(result_both_check_label);

  // result = 0|1
  int result_after_label = 1 - result_after_two_checks;
  instructions_.emplace_back(
    make_tacky<TackyCopy>(make_tacky<TackyConstant>(result_after_label), result));

  // Label(end)
  instructions_.emplace_back(end_label);
  return result;
}

std::shared_ptr<Tacky> TackyGen::operator()(const BinaryExpr& expr) {
  // binary_operator = Add | Subtract | Multiply | Divide | Remainder | Equal |
  // NotEqual | LessThan | LessOrEqual | GreaterThan | GreaterOrEqual
  TokenType op = expr.Operator.type;
  bool isLogical = isLogicalOp(op);
  assert(one_of(op, {TokenType::PLUS, TokenType::MINUS, TokenType::STAR,
                TokenType::SLASH, TokenType::PERCENT}) ||
         isLogical || isRelationalOp(op));
  
  // Logical operations are short circuited.
  if (isLogical) {
    return genLogical(expr);
  }

  // v1 = emit_tacky(e1, instructions)
  std::shared_ptr<Tacky> src1 = gen(expr.left.get());

  // v2 = emit_tacky(e2, instructions)
  std::shared_ptr<Tacky> src2 = gen(expr.right.get());

  // dst_name = make_temporary()
  // dst = Var(dst_name)
  auto dst = make_tacky<TackyVar>(unique_var());

  // tacky_op = convert_binop(op)
  // instructions.append(Binary(tacky_op, v1, v2, dst))
  // NOTE: tacky_op and expr->Operator are the same.
  instructions_.emplace_back(
    make_tacky<TackyBinary>(expr.Operator, src1, src2, dst));
  return dst;
}

std::shared_ptr<Tacky> TackyGen::operator()(const LiteralExpr& expr) {
  assert(expr.type == TokenType::NUMBER);
  auto lexpr = make_tacky<TackyConstant>(std::stoi(expr.value));
  instructions_.emplace_back(lexpr);
  return lexpr;
}

std::shared_ptr<Tacky> TackyGen::operator()(const UnaryExpr& expr) {
  // unary_operator = Complement | Negate | Not
  assert(one_of(expr.Operator.type, {TokenType::TILDE, TokenType::MINUS,
                TokenType::BANG}));

  // src = emit_tacky(inner, instructions)
  std::shared_ptr<Tacky> src = gen(expr.right.get());

  // dst_name = make_temporary()
  // dst = Var(dst_name)
  auto dst = make_tacky<TackyVar>(unique_var());

  // tacky_op = convert_unop(op)
  // instructions.append(Unary(tacky_op, src, dst))
  // NOTE: tacky_op and expr->Operator are the same.
  instructions_.emplace_back(
    make_tacky<TackyUnary>(expr.Operator, src, dst));

  return dst;
}

std::shared_ptr<Tacky> TackyGen::operator()(const Variable& var) {
  return make_tacky<TackyVar>(std::format("{}_scope_level{}", var.name.toString(), var.level));
}
