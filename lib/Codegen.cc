#include "Codegen.h"
#include "Token.h"
#include <cassert>
#include <format>
#include <sstream>
#include <unordered_map>

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
    auto inst_code = std::any_cast<std::string>(inst->accept(*this));
    // need special handling for assembly labels
    if (std::dynamic_pointer_cast<AsmLabel>(inst)) {
      ss << inst_code << ":\n";
    } else {
      ss << "  " << inst_code << '\n';
    }
  }
  return ss.str();
}

std::any Codegen::visitAsmUnary(std::shared_ptr<AsmUnary> unary) {
  auto operand = code(unary->operand);
  switch (unary->op.type) {
    case TokenType::MINUS:
      return std::format("negl {}", operand);
    case TokenType::TILDE:
      return std::format("notl {}", operand);
    default:
      assert(0);
      break;
  }
  return nullptr;
}

std::any Codegen::visitAsmBinary(std::shared_ptr<AsmBinary> bin) {
  auto operand1 = code(bin->operand1);
  auto operand2 = code(bin->operand2);
  switch (bin->op.type) {
    case TokenType::PLUS:
      return std::format("addl {}, {}", operand1, operand2);
    case TokenType::MINUS:
      return std::format("subl {}, {}", operand1, operand2);
    case TokenType::STAR:
      return std::format("imull {}, {}", operand1, operand2);
    default:
      assert(0);
      break;
  }
  return nullptr;
}

std::any Codegen::visitAsmCmp(std::shared_ptr<AsmCmp> cmp) {
  auto operand1 = code(cmp->operand1);
  auto operand2 = code(cmp->operand2);
  return std::format("cmpl {}, {}", operand1, operand2);
}

std::any Codegen::visitAsmIdiv(std::shared_ptr<AsmIdiv> idiv) {
  auto operand = code(idiv->operand);
  return std::format("idivl {}", operand);
}

std::any Codegen::visitAsmCdq(std::shared_ptr<AsmCdq>) {
  return std::format("cdq");
}

std::any Codegen::visitAsmJmp(std::shared_ptr<AsmJmp> jmp) {
  auto target = code(jmp->target);
  return std::format("jmp {}", target);
}

std::any Codegen::visitAsmJmpCC(std::shared_ptr<AsmJmpCC> jmpcc) {
  static std::unordered_map<Asm::CondCode, std::string> code_to_inst =
    {{Asm::CondCode::E, "je"},
     {Asm::CondCode::NE, "jne"},
     {Asm::CondCode::L, "jl"},
     {Asm::CondCode::LE, "jle"},
     {Asm::CondCode::G, "jg"},
     {Asm::CondCode::GE, "jge"}};

  auto target = code(jmpcc->target);
  auto it = code_to_inst.find(jmpcc->cond_code);
  if (it != code_to_inst.end()) {
    return std::format("{} {}", it->second, target);
  } else {
    errorHandler_.add(0, "asm jmp gen", "");
    return nullptr;
  }
}

std::any Codegen::visitAsmSetCC(std::shared_ptr<AsmSetCC> setcc) {
  static std::unordered_map<Asm::CondCode, std::string> code_to_inst =
    {{Asm::CondCode::E, "sete"},
     {Asm::CondCode::NE, "setne"},
     {Asm::CondCode::L, "setl"},
     {Asm::CondCode::LE, "setle"},
     {Asm::CondCode::G, "setg"},
     {Asm::CondCode::GE, "setge"}};

  auto operand = code(setcc->operand);
  auto it = code_to_inst.find(setcc->cond_code);
  if (it != code_to_inst.end()) {
    return std::format("{} {}", it->second, operand);
  } else {
    errorHandler_.add(0, "asm setcc gen", "");
    return nullptr;
  }
}

std::any Codegen::visitAsmLabel(std::shared_ptr<AsmLabel> label) {
  return std::format(".L_{}", label->identifier);
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
    case Asm::Reg::AX:
      return std::string("%eax");
    /*
    case Asm::Reg::BX:
      return std::string("%ebx");
    case 2:
      return std::string("%ecx");
    */
    case Asm::Reg::DX:
      return std::string("%edx");
    case Asm::Reg::R10:
      return std::string("%r10d");
    case Asm::Reg::R11:
      return std::string("%r11d");
    default:
      return nullptr;
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
