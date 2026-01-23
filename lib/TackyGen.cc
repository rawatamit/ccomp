#include "TackyGen.h"
#include "Token.h"
#include "ast/Asm.h"
#include "ast/Tacky.h"
#include "Util.h"
#include <any>
#include <cassert>
#include <format>
#include <iterator>
#include <memory>
#include <vector>

using namespace ccomp;

TackyGen::TackyGen(const std::vector<std::shared_ptr<Stmt>>& stmts, ErrorHandler &errorHandler) :
  stmts_(stmts), errorHandler_(errorHandler)
{}

std::shared_ptr<Tacky> TackyGen::gen() {
  std::vector<std::shared_ptr<Tacky>> fns;
  for (auto stmt : stmts_) {
    auto fn = std::any_cast<std::shared_ptr<TackyFunction>>(stmt->accept(*this));
    fns.push_back(fn);
  }
  return std::make_shared<TackyProgram>(fns);
}

std::vector<std::shared_ptr<Tacky>> TackyGen::gen(std::shared_ptr<Expr> expr) {
  auto val = expr->accept(*this);
  return std::any_cast<std::vector<std::shared_ptr<Tacky>>>(val);
}

std::vector<std::shared_ptr<Tacky>> TackyGen::gen(std::shared_ptr<Stmt> stmt) {
  auto val = stmt->accept(*this);
  return std::any_cast<std::vector<std::shared_ptr<Tacky>>>(val);
}

std::vector<std::shared_ptr<Tacky>> TackyGen::gen(std::vector<std::shared_ptr<Expr>> exprs) {
  std::vector<std::shared_ptr<Tacky>> genexprs;
  for (auto expr : exprs) {
    auto genexpr = gen(expr);
    std::copy(genexpr.begin(), genexpr.end(), std::back_inserter(genexprs));
  }
  return genexprs;
}

std::vector<std::shared_ptr<Tacky>> TackyGen::gen(std::vector<std::shared_ptr<Stmt>> stmts) {
  std::vector<std::shared_ptr<Tacky>> genstmts;
  for (auto stmt : stmts) {
    auto genexpr = gen(stmt);
    std::copy(genexpr.begin(), genexpr.end(), std::back_inserter(genstmts));
  }
  return genstmts;
}

std::string TackyGen::unique_var() {
  static int nextId = 0;
  return std::format("tmp.{}", nextId++);
}

std::shared_ptr<TackyLabel> TackyGen::unique_label(const std::string& desc) {
  static int nextId = 0;
  return std::make_shared<TackyLabel>(std::format("T{}.{}", desc, nextId++));
}

std::any TackyGen::visitBlock(std::shared_ptr<Block> stmt) {
  return gen(stmt->stmts);
}

std::any TackyGen::visitExpression(std::shared_ptr<Expression> stmt) {
  return gen(stmt->expr);
}

std::any TackyGen::visitFunction(std::shared_ptr<Function> fn) {
  std::vector<std::shared_ptr<Tacky>> insts;
  for (auto p : fn->body) {
    std::vector<std::shared_ptr<Tacky>> asmcode = gen(p);
    std::copy(asmcode.begin(), asmcode.end(), std::back_inserter(insts));
  }

  return std::make_shared<TackyFunction>(fn->name, insts);
}

std::any TackyGen::visitIf(std::shared_ptr<If> stmt) {
  printf("(if ");
  gen(stmt->condition);
  return stmt->thenBranch, stmt->elseBranch;
}

std::any TackyGen::visitPrint(std::shared_ptr<Print> stmt) {
  return stmt->expr;
}

std::any TackyGen::visitReturn(std::shared_ptr<Return> ret) {
  auto expr = gen(ret->value);
  // convert constant or var to a return expression
  expr.back() = std::make_shared<TackyReturn>(expr.back());
  return expr;
}

std::any TackyGen::visitWhile(std::shared_ptr<While> Stmt) {
  gen(Stmt->condition);
  gen(Stmt->body);
  return nullptr;
}

std::any TackyGen::visitVar(std::shared_ptr<Var> Stmt) {
  return Stmt->name.lexeme, Stmt->init;
}

std::any TackyGen::visitAssign(std::shared_ptr<Assign> expr) {
  return expr->name.lexeme, expr->value;
}

std::any TackyGen::genLogical(std::shared_ptr<BinaryExpr> expr) {
  TokenType op = expr->Operator.type;
  std::vector<std::shared_ptr<Tacky>> insts;
  // <instructions for e1>
  std::vector<std::shared_ptr<Tacky>> left_code =
    (std::any_cast<std::vector<std::shared_ptr<Tacky>>>(gen(expr->left)));
  std::copy(left_code.begin(), left_code.end() - 1, std::back_inserter(insts));

  // v1 = <result of e1>
  std::shared_ptr<Tacky> v1 = left_code.back();

  std::shared_ptr<TackyLabel> cond_true_label = unique_label("logical");
  std::shared_ptr<TackyLabel> end_label = unique_label("logical");

  // JumpIfZero|JumpIfNotZero(v1, cond_true_label)
  if (op == TokenType::AMPERSAND_AMPERSAND) {
    insts.emplace_back(std::make_shared<TackyJumpIfZero>(v1, cond_true_label));
  } else if (op == TokenType::PIPE_PIPE) {
    insts.emplace_back(std::make_shared<TackyJumpIfNotZero>(v1, cond_true_label));
  }

  // <instructions for e2>
  std::vector<std::shared_ptr<Tacky>> right_code =
    (std::any_cast<std::vector<std::shared_ptr<Tacky>>>(gen(expr->right)));
  std::copy(right_code.begin(), right_code.end() - 1, std::back_inserter(insts));

  // v2 = <result of e2>
  std::shared_ptr<Tacky> v2 = right_code.back();

  // JumpIfZero|JumpIfNotZero(v2, cond_true_label)
  if (op == TokenType::AMPERSAND_AMPERSAND) {
    insts.emplace_back(std::make_shared<TackyJumpIfZero>(v2, cond_true_label));
  } else if (op == TokenType::PIPE_PIPE) {
    insts.emplace_back(std::make_shared<TackyJumpIfNotZero>(v2, cond_true_label));
  }

  // result = 1|0
  // && returns a 1 if both conditions were true. || returns a 0 if both
  // conditions are false. Note we are using JumpIfZero for && and JumpIfNotZero
  // for ||.
  int result_after_two_checks = (op == TokenType::AMPERSAND_AMPERSAND) ? 1 : 0;
  auto result = std::make_shared<TackyVar>(unique_var());
  insts.emplace_back(std::make_shared<TackyCopy>(
    std::make_shared<TackyConstant>(result_after_two_checks), result));

  // Jump(end)
  insts.emplace_back(std::make_shared<TackyJump>(end_label));

  // Label(cond_true_label)
  insts.emplace_back(cond_true_label);

  // result = 0|1
  int result_after_label = 1 - result_after_two_checks;
  insts.emplace_back(std::make_shared<TackyCopy>(std::make_shared<TackyConstant>(result_after_label), result));

  // Label(end)
  insts.emplace_back(end_label);

  insts.emplace_back(result);
  return insts;
}

std::any TackyGen::visitBinaryExpr(std::shared_ptr<BinaryExpr> expr) {
  // binary_operator = Add | Subtract | Multiply | Divide | Remainder | Equal |
  // NotEqual | LessThan | LessOrEqual | GreaterThan | GreaterOrEqual
  TokenType op = expr->Operator.type;
  bool isLogical = isLogicalOp(op);
  assert(one_of(op, {TokenType::PLUS, TokenType::MINUS, TokenType::STAR,
                TokenType::SLASH, TokenType::PERCENT}) ||
         isLogical || isRelationalOp(op));
  
  // Logical operations are short circuited.
  if (isLogical) {
    return genLogical(expr);
  }

  std::vector<std::shared_ptr<Tacky>> insts;
  // v1 = emit_tacky(e1, instructions)
  std::vector<std::shared_ptr<Tacky>> left_code =
    (std::any_cast<std::vector<std::shared_ptr<Tacky>>>(gen(expr->left)));

  // source var is at the end of the code
  std::shared_ptr<Tacky> src1 = left_code.back();

  // copy while skipping last entry, which is the variable
  std::copy(left_code.begin(), left_code.end() - 1, std::back_inserter(insts));

  // v2 = emit_tacky(e2, instructions)
  std::vector<std::shared_ptr<Tacky>> right_code =
    (std::any_cast<std::vector<std::shared_ptr<Tacky>>>(gen(expr->right)));

  // source var is at the end of the code
  std::shared_ptr<Tacky> src2 = right_code.back();

  // copy while skipping last entry, which is the variable
  std::copy(right_code.begin(), right_code.end() - 1, std::back_inserter(insts));

  // dst_name = make_temporary()
  // dst = Var(dst_name)
  auto dst = std::make_shared<TackyVar>(unique_var());

  // tacky_op = convert_binop(op)
  // instructions.append(Binary(tacky_op, v1, v2, dst))
  // NOTE: tacky_op and expr->Operator are the same.
  insts.emplace_back(std::make_shared<TackyBinary>(expr->Operator, src1, src2, dst));

  // return dst
  insts.emplace_back(dst);
  return insts;
}

std::any TackyGen::visitLiteralExpr(std::shared_ptr<LiteralExpr> expr) {
  assert(expr->type == TokenType::NUMBER);
  return std::vector<std::shared_ptr<Tacky>>({
    std::make_shared<TackyConstant>(std::stoi(expr->value))});
}

std::any TackyGen::visitUnaryExpr(std::shared_ptr<UnaryExpr> expr) {
  // unary_operator = Complement | Negate | Not
  assert(one_of(expr->Operator.type, {TokenType::TILDE, TokenType::MINUS,
                TokenType::BANG}));
  std::vector<std::shared_ptr<Tacky>> insts;

  // src = emit_tacky(inner, instructions)
  std::vector<std::shared_ptr<Tacky>> asm_code =
    (std::any_cast<std::vector<std::shared_ptr<Tacky>>>(gen(expr->right)));
  // source var is at the end of the code
  std::shared_ptr<Tacky> src = asm_code.back();

  // copy while skipping last entry, which is the variable
  std::copy(asm_code.begin(), asm_code.end() - 1, std::back_inserter(insts));

  // dst_name = make_temporary()
  // dst = Var(dst_name)
  auto dst = std::make_shared<TackyVar>(unique_var());

  // tacky_op = convert_unop(op)
  // instructions.append(Unary(tacky_op, src, dst))
  // NOTE: tacky_op and expr->Operator are the same.
  insts.emplace_back(std::make_shared<TackyUnary>(expr->Operator, src, dst));

  // return dst
  insts.emplace_back(dst);
  return insts;
}

std::any TackyGen::visitVariable(std::shared_ptr<Variable> expr) {
  return expr->name.lexeme;
}
