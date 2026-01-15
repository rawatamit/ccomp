#include "Codegen.h"
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
  for (auto inst : fn->instructions) {
    ss << "  " << std::any_cast<std::string>(inst->accept(*this)) << '\n';
  }
  return ss.str();
}

std::any Codegen::visitAsmBinaryInst(std::shared_ptr<AsmBinaryInst> bin __attribute_maybe_unused__) {
  return nullptr;
}

std::any Codegen::visitAsmMov(std::shared_ptr<AsmMov> mov) {
  auto src = code(mov->src);
  auto dest = code(mov->dest);
  return std::format("movl {}, {}", src, dest);
}

std::any Codegen::visitAsmImm(std::shared_ptr<AsmImm> imm) {
  return std::format("${}", imm->operand.c_str());
}

std::any Codegen::visitAsmReturn(std::shared_ptr<AsmReturn> ret __attribute_maybe_unused__) {
  return std::string("ret");
}

std::any Codegen::visitAsmRegister(std::shared_ptr<AsmRegister> reg __attribute_maybe_unused__) {
  return std::string("%eax");
}
