#include "Codegen.h"
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

std::string Codegen::toInst(AsmInst op, AsmInstType type) const {
  static std::unordered_map<AsmInst, std::string> opToInst = {
    {AsmInst::ADD, "add"},
    {AsmInst::SUB, "sub"},
    {AsmInst::MUL, "imul"},
    {AsmInst::NEG, "neg"},
    {AsmInst::NOT, "not"},
    {AsmInst::CALL, "call"},
    {AsmInst::CMP, "cmp"},
    {AsmInst::DIV, "idiv"},
    {AsmInst::MOV, "mov"},
  };

  if (op == AsmInst::CDQ) {
    switch (type) {
    case BYTE:
      return "cwd";
    case LONG:
      return "cdq";
    case QUAD:
      return "cqo";
    default:
      return "cdq::err";
    }
  } else if (op == AsmInst::PUSH) {
    return "pushq";
  } else if (op == AsmInst::JMP) {
    return "jmp";
  } else {
    auto it = opToInst.find(op);
    return addTypeSuffix(it->second, type);
  }
}

std::string Codegen::addTypeSuffix(const std::string& inst, AsmInstType type) const {
  if (type == AsmInstType::LONG) {
    return std::format("{}l", inst);
  } else if (type == AsmInstType::QUAD) {
    return std::format("{}q", inst);
  }

  return "AsmInstType::ERROR";
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
  const std::string& name = fn.name;

  if (fn.global) {
    ss << "  .globl " << name << '\n';
  }

  ss << "  .text\n";
  ss << name << ":\n";
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

std::string Codegen::operator()(const AsmStaticVar& svar) {
  std::stringstream ss;
  const std::string& name = svar.name;
  if (svar.global) {
    ss << "  .globl " << name << '\n';
  }
  
  if (svar.init.getValue() != 0) {
    ss << "  .data\n";
    ss << "  .align " << svar.alignment << '\n';
    ss << name << ":\n";

    if (svar.init.getType() == InitialValue::INITIAL_INT32_VALUE) {
      ss << "  .long ";
    } else {
      assert(svar.init.getType() == InitialValue::INITIAL_LONG_VALUE);
      ss << "  .quad "; 
    }

    ss << svar.init.getValue() << '\n';
  } else {
    ss << "  .bss\n";
    ss << "  .align " << svar.alignment << '\n';
    ss << name << ":\n";

    ss << "  .zero";
    if (svar.init.getType() == InitialValue::INITIAL_INT32_VALUE) {
      ss << "  4\n";
    } else {
      ss << "  8\n";
    }
  }

  return ss.str();
}

std::string Codegen::operator()(const AsmUnary& unary) {
  auto operand = code(unary.operand);
  std::string inst = toInst(unary.op, unary.type);
  return std::format("{} {}", inst, operand);
}

std::string Codegen::operator()(const AsmBinary& bin) {
  auto operand1 = code(bin.operand1);
  auto operand2 = code(bin.operand2);
  std::string inst = toInst(bin.op, bin.type);
  return std::format("{} {}, {}", inst, operand1, operand2);
}

std::string Codegen::operator()(const AsmCmp& cmp) {
  auto operand1 = code(cmp.operand1);
  auto operand2 = code(cmp.operand2);
  std::string inst = toInst(AsmInst::CMP, cmp.type);
  return std::format("{} {}, {}", inst, operand1, operand2);
}

std::string Codegen::operator()(const AsmIdiv& idiv) {
  auto operand = code(idiv.operand);
  std::string inst = toInst(AsmInst::DIV, idiv.type);
  return std::format("{} {}", inst, operand);
}

std::string Codegen::operator()(const AsmCdq& cdq) {
  return toInst(AsmInst::CDQ, cdq.type);
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
  std::string inst = toInst(AsmInst::MOV, mov.type);
  return std::format("{} {}, {}", inst, src, dest);
}

std::string Codegen::operator()(const AsmMovsx& mov) {
  auto src = code(mov.src);
  auto dest = code(mov.dest);
  return std::format("movslq {}, {}", src, dest);
}

std::string Codegen::operator()(const AsmPush& push) {
  auto operand = code(push.operand);
  return std::format("pushq {}", operand);
}

std::string Codegen::operator()(const AsmCall& call) {
  return std::format("call {}@PLT", call.fname);
}

std::string Codegen::operator()(const AsmImm& imm) {
  return std::format("${}", imm.value);
}

std::string Codegen::operator()(const AsmReturn&) {
  return std::format("movq %rbp, %rsp\n  popq %rbp\n  ret");
}

std::string Codegen::operator()(const AsmRegister& reg) {
  static std::unordered_map<AsmReg,
          std::unordered_map<AsmInstType, std::string>> regmap = {
    {AsmReg::AX,
          {{AsmInstType::QUAD, "%rax"},
           {AsmInstType::LONG, "%eax"},
           {AsmInstType::BYTE, "%al"}}},

    {AsmReg::CX,
          {{AsmInstType::QUAD, "%rcx"},
           {AsmInstType::LONG, "%ecx"},
           {AsmInstType::BYTE, "%cl"}}},

    {AsmReg::DX,
          {{AsmInstType::QUAD, "%rdx"},
           {AsmInstType::LONG, "%edx"},
           {AsmInstType::BYTE, "%dl"}}},

    {AsmReg::DI,
          {{AsmInstType::QUAD, "%rdi"},
           {AsmInstType::LONG, "%edi"},
           {AsmInstType::BYTE, "%dil"}}},

    {AsmReg::SI,
          {{AsmInstType::QUAD, "%rsi"},
           {AsmInstType::LONG, "%esi"},
           {AsmInstType::BYTE, "%sil"}}},

    {AsmReg::R8,
          {{AsmInstType::QUAD, "%r8"},
           {AsmInstType::LONG, "%r8d"},
           {AsmInstType::BYTE, "%r8b"}}},

    {AsmReg::R9,
          {{AsmInstType::QUAD, "%r9"},
           {AsmInstType::LONG, "%r9d"},
           {AsmInstType::BYTE, "%r9b"}}},

    {AsmReg::R10,
          {{AsmInstType::QUAD, "%r10"},
           {AsmInstType::LONG, "%r10d"},
           {AsmInstType::BYTE, "%r10b"}}},

    {AsmReg::R11,
          {{AsmInstType::QUAD, "%r11"},
           {AsmInstType::LONG, "%r11d"},
           {AsmInstType::BYTE, "%r11b"}}},

    {AsmReg::SP,
          {{AsmInstType::QUAD, "%rsp"},
           {AsmInstType::LONG, "%esp"},
           {AsmInstType::BYTE, "%spl"}}}};

  // find register
  auto regnames = regmap.find(reg.reg); 
  if (regnames != regmap.end()) {
    // find name based on size
    auto it = regnames->second.find(reg.type);
    if (it != regnames->second.end()) {
      return it->second;
    }
  }

  return nullptr;
}

std::string Codegen::operator()(const AsmPseudo&) {
  assert(0);
  return nullptr;
}

std::string Codegen::operator()(const AsmStack& st) {
  // -ve offset from rbp
  return std::format("{}(%rbp)", st.offset);
}

std::string Codegen::operator()(const AsmData& data) {
  return std::format("{}(%rip)", data.identifier);
}
