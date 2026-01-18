#include "Codegen.h"
#include "Token.h"
#include <cassert>
#include <format>
#include <sstream>

using namespace ccomp;

Codegen::Codegen(std::shared_ptr<Asm> program,
                 ErrorHandler& errorHandler) :
  program_(program), errorHandler_(errorHandler)
{}

std::string Codegen::code() {
  return code(program_);
}

std::string Codegen::code(std::shared_ptr<Asm> expr) {
  return std::any_cast<std::string>(expr->accept(*this));
}

std::string Codegen::code(std::vector<std::shared_ptr<Asm>> insts) {
  std::stringstream ss;
  for (auto inst : insts) {
    ss << code(inst);
  }
  return ss.str();
}

std::any Codegen::visitAsmProgram(std::shared_ptr<AsmProgram> prog) {
  std::stringstream ss;
  for (auto fn : prog->functions) {
    ss << std::any_cast<std::string>(fn->accept(*this)) << '\n';
  }
  ss << ".section .note.GNU-stack,\"\",@progbits";
  return ss.str();
}

std::any Codegen::visitAsmFunction(std::shared_ptr<AsmFunction> fn) {
  std::stringstream ss;
  auto name = fn->name.toString();
  ss << ".globl " << name << '\n'
     << name << ":\n";
  ss << "  pushq %rbp\n  movq %rsp, %rbp\n";
  for (auto inst : fn->instructions) {
    ss << "  " << std::any_cast<std::string>(inst->accept(*this)) << '\n';
  }
  return ss.str();
}

std::any Codegen::visitAsmUnary(std::shared_ptr<AsmUnary> unary) {
  auto operand = code(unary->operand);
  switch (unary->op.type) {
    case ccomp::TokenType::MINUS:
      return std::format("negl {}", operand);
    case ccomp::TokenType::TILDE:
      return std::format("notl {}", operand);
    default:
      assert(0);
      break;
  }
  return nullptr;
}

std::any Codegen::visitAsmMov(std::shared_ptr<AsmMov> mov) {
  auto src = code(mov->src);
  auto dest = code(mov->dest);
  return std::format("movl {}, {}", src, dest);
}

std::any Codegen::visitAsmAllocateStack(std::shared_ptr<AsmAllocateStack> alloc) {
  return std::format("subq ${}, %rsp", alloc->size);
}

std::any Codegen::visitAsmImm(std::shared_ptr<AsmImm> imm) {
  return std::format("${}", imm->value);
}

std::any Codegen::visitAsmReturn(std::shared_ptr<AsmReturn> ret __attribute_maybe_unused__) {
  return std::format("movq %rbp, %rsp\n  popq %rbp\n  ret");
}

std::any Codegen::visitAsmRegister(std::shared_ptr<AsmRegister> reg) {
  switch (reg->reg) {
    case 0:
      return std::string("%eax");
    case 10:
      return std::string("%r10d");
    default:
      assert(0);
      break;
  }
  return nullptr;
}

std::any Codegen::visitAsmPseudo(std::shared_ptr<AsmPseudo>) {
  assert(0);
  return nullptr;
}

std::any Codegen::visitAsmStack(std::shared_ptr<AsmStack> st) {
  // -ve offset from rbp
  return std::format("-{}(%rbp)", st->offset);
}
