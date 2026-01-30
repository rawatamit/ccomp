#include "Codegen.h"
#include "Token.h"
#include <cassert>
#include <format>
#include <sstream>
#include <unordered_map>

using namespace ccomp;

Codegen::Codegen(const Asm* program,
                 ErrorHandler& errorHandler) :
  program_(program), errorHandler_(errorHandler)
{}

std::string Codegen::code() {
  return std::visit(*this, *program_);
}

std::string Codegen::code(std::shared_ptr<Asm> expr) {
  return std::visit(*this, *expr);
}

std::string Codegen::code(std::vector<std::shared_ptr<Asm>> insts) {
  std::stringstream ss;
  for (auto inst : insts) {
    ss << code(inst);
  }
  return ss.str();
}

std::string Codegen::operator()(const AsmProgram& prog) {
  std::stringstream ss;
  for (auto fn : prog.functions) {
    ss << code(fn) << '\n';
  }
  ss << ".section .note.GNU-stack,\"\",@progbits";
  return ss.str();
}

std::string Codegen::operator()(const AsmFunction& fn) {
  std::stringstream ss;
  auto name = fn.name.toString();
  ss << ".globl " << name << '\n'
     << name << ":\n";
  ss << "  pushq %rbp\n  movq %rsp, %rbp\n";
  for (auto inst : fn.instructions) {
    auto inst_code = code(inst);
    // need special handling for assembly labels
    if (std::holds_alternative<AsmLabel>(*inst)) {
      ss << inst_code << ":\n";
    } else {
      ss << "  " << inst_code << '\n';
    }
  }
  return ss.str();
}

std::string Codegen::operator()(const AsmUnary& unary) {
  auto operand = code(unary.operand);
  switch (unary.op.type) {
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

std::string Codegen::operator()(const AsmBinary& bin) {
  auto operand1 = code(bin.operand1);
  auto operand2 = code(bin.operand2);
  switch (bin.op.type) {
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

std::string Codegen::operator()(const AsmCmp& cmp) {
  auto operand1 = code(cmp.operand1);
  auto operand2 = code(cmp.operand2);
  return std::format("cmpl {}, {}", operand1, operand2);
}

std::string Codegen::operator()(const AsmIdiv& idiv) {
  auto operand = code(idiv.operand);
  return std::format("idivl {}", operand);
}

std::string Codegen::operator()(const AsmCdq&) {
  return std::format("cdq");
}

std::string Codegen::operator()(const AsmJmp& jmp) {
  return std::format("jmp {}", code(jmp.target));
}

std::string Codegen::operator()(const AsmJmpCC& jmpcc) {
  static std::unordered_map<AsmCondCode, std::string> code_to_inst =
    {{AsmCondCode::E, "je"},
     {AsmCondCode::NE, "jne"},
     {AsmCondCode::L, "jl"},
     {AsmCondCode::LE, "jle"},
     {AsmCondCode::G, "jg"},
     {AsmCondCode::GE, "jge"}};

  auto it = code_to_inst.find(jmpcc.cond_code);
  if (it != code_to_inst.end()) {
    return std::format("{} {}", it->second, code(jmpcc.target));
  } else {
    errorHandler_.add(0, "asm jmp gen", "");
    return nullptr;
  }
}

std::string Codegen::operator()(const AsmSetCC& setcc) {
  static std::unordered_map<AsmCondCode, std::string> code_to_inst =
    {{AsmCondCode::E, "sete"},
     {AsmCondCode::NE, "setne"},
     {AsmCondCode::L, "setl"},
     {AsmCondCode::LE, "setle"},
     {AsmCondCode::G, "setg"},
     {AsmCondCode::GE, "setge"}};

  auto operand = code(setcc.operand);
  auto it = code_to_inst.find(setcc.cond_code);
  if (it != code_to_inst.end()) {
    return std::format("{} {}", it->second, operand);
  } else {
    errorHandler_.add(0, "asm setcc gen", "");
    return nullptr;
  }
}

std::string Codegen::operator()(const AsmLabel& label) {
  return std::format(".L_{}", label.identifier);
}

std::string Codegen::operator()(const AsmMov& mov) {
  auto src = code(mov.src);
  auto dest = code(mov.dest);
  return std::format("movl {}, {}", src, dest);
}

std::string Codegen::operator()(const AsmAllocateStack& alloc) {
  return std::format("subq ${}, %rsp", alloc.size);
}

std::string Codegen::operator()(const AsmImm& imm) {
  return std::format("${}", imm.value);
}

std::string Codegen::operator()(const AsmReturn&) {
  return std::format("movq %rbp, %rsp\n  popq %rbp\n  ret");
}

std::string Codegen::operator()(const AsmRegister& reg) {
  switch (reg.reg) {
    case AsmReg::AX:
      return std::string("%eax");
    /*
    case Asm::Reg::BX:
      return std::string("%ebx");
    case 2:
      return std::string("%ecx");
    */
    case AsmReg::DX:
      return std::string("%edx");
    case AsmReg::R10:
      return std::string("%r10d");
    case AsmReg::R11:
      return std::string("%r11d");
    default:
      return nullptr;
  }
  return nullptr;
}

std::string Codegen::operator()(const AsmPseudo&) {
  assert(0);
  return nullptr;
}

std::string Codegen::operator()(const AsmStack& st) {
  // -ve offset from rbp
  return std::format("-{}(%rbp)", st.offset);
}
