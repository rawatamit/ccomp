#include "AsmGen.h"
#include "Token.h"
#include "ast/Asm.h"
#include <any>
#include <cassert>
#include <memory>
#include <vector>

using namespace ccomp;

AsmGen::AsmGen(const std::vector<std::shared_ptr<Stmt>>& stmts, ErrorHandler &errorHandler) :
  stmts_(stmts), errorHandler_(errorHandler)
{}

std::shared_ptr<Asm> AsmGen::gen() {
  return std::make_shared<AsmProgram>(gen(stmts_));
}

std::vector<std::shared_ptr<Asm>> AsmGen::gen(std::shared_ptr<Expr> expr) {
  auto val = expr->accept(*this);
  return std::any_cast<std::vector<std::shared_ptr<Asm>>>(val);
}

std::vector<std::shared_ptr<Asm>> AsmGen::gen(std::shared_ptr<Stmt> stmt) {
  auto val = stmt->accept(*this);
  return std::any_cast<std::vector<std::shared_ptr<Asm>>>(val);
}

std::vector<std::shared_ptr<Asm>> AsmGen::gen(std::vector<std::shared_ptr<Expr>> exprs) {
  std::vector<std::shared_ptr<Asm>> genexprs;
  for (auto expr : exprs) {
    auto genexpr(std::any_cast<std::vector<std::shared_ptr<Asm>>>(gen(expr)));
    for (auto inst : genexpr) {
      genexprs.emplace_back(inst);
    }
  }
  return genexprs;
}

std::vector<std::shared_ptr<Asm>> AsmGen::gen(std::vector<std::shared_ptr<Stmt>> stmts) {
  std::vector<std::shared_ptr<Asm>> genstmts;
  for (auto stmt : stmts) {
    auto genexpr(std::any_cast<std::vector<std::shared_ptr<Asm>>>(gen(stmt)));
    for (auto inst : genexpr) {
      genstmts.emplace_back(inst);
    }
  }
  return genstmts;
}

std::any AsmGen::visitBlock(std::shared_ptr<Block> stmt) {
  return gen(stmt->stmts);
  // return parenthesizeS("block", stmt->stmts);
}

std::any AsmGen::visitExpression(std::shared_ptr<Expression> stmt) {
  return gen(stmt->expr);
}

std::any AsmGen::visitFunction(std::shared_ptr<Function> fn) {
  // for (auto p : stmt->params)
  std::vector<std::shared_ptr<Asm>> insts;
  for (auto p : fn->body) {
    std::vector<std::shared_ptr<Asm>> asmcode =
      (std::any_cast<std::vector<std::shared_ptr<Asm>>>(gen(p)));
    for (auto inst: asmcode) {
      insts.emplace_back(inst);
    }
  }

  return std::vector<std::shared_ptr<Asm>>(
    {std::make_shared<AsmFunction>(fn->name, insts)});
  //return stmt->body;
}

std::any AsmGen::visitIf(std::shared_ptr<If> stmt) {
  printf("(if ");
  gen(stmt->condition);
  return stmt->thenBranch, stmt->elseBranch;
}

std::any AsmGen::visitPrint(std::shared_ptr<Print> stmt) {
  return stmt->expr;
}

template <typename Arg>
std::vector<std::shared_ptr<Asm>> make_vec(Arg&& list) {
  return list;
}
//  auto expr = gen(Stmt->value);
//  std::vector<std::shared_ptr<Asm>> insts;
//  insts.emplace_back(std::make_shared<AsmMov>(expr, std::make_shared<AsmRegister>(0)));
//  insts.emplace_back(std::make_shared<AsmReturn>(0));
//  return insts;
//}

std::any AsmGen::visitReturn(std::shared_ptr<Return> Stmt) {
  //return std::vector<std::shared_ptr<Asm>>({std::make_shared<AsmReturn>(0), std::make_shared<AsmMov>(0,0)});
  auto expr = gen(Stmt->value);
  return std::vector<std::shared_ptr<Asm>>({
    std::make_shared<AsmMov>(expr, std::make_shared<AsmRegister>(0)),
    std::make_shared<AsmReturn>(0)});
}

std::any AsmGen::visitWhile(std::shared_ptr<While> Stmt) {
  gen(Stmt->condition);
  gen(Stmt->body);
  return nullptr;
}

std::any AsmGen::visitVar(std::shared_ptr<Var> Stmt) {
  return Stmt->name.lexeme, Stmt->init;
}

std::any AsmGen::visitAssign(std::shared_ptr<Assign> expr) {
  return expr->name.lexeme, expr->value;
}

std::any AsmGen::visitBinaryExpr(std::shared_ptr<BinaryExpr> expr) {
  return expr->Operator.lexeme, expr->left, expr->right;
}

std::any AsmGen::visitLogical(std::shared_ptr<Logical> expr) {
  return expr->Operator.lexeme, expr->left, expr->right;
}

std::any AsmGen::visitLiteralExpr(std::shared_ptr<LiteralExpr> expr) {
  assert(expr->type == TokenType::NUMBER);
  return std::vector<std::shared_ptr<Asm>>({
    std::make_shared<AsmImm>(expr->type, expr->value)});
}

std::any AsmGen::visitUnaryExpr(std::shared_ptr<UnaryExpr> expr) {
  return expr->Operator.lexeme, expr->right;
}

std::any AsmGen::visitVariable(std::shared_ptr<Variable> expr) {
  return expr->name.lexeme;
}
