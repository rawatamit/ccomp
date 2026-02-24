#include "AsmGen.h"
#include "ast/Asm.h"
#include "Util.h"
#include <cassert>
#include <memory>
#include <unordered_map>
#include <variant>
#include <vector>

using namespace ccomp;

const std::vector<AsmReg> AsmGen::arg_regs_ = {DI, SI, DX, CX, R8, R9};

AsmGen::AsmGen(Tacky* tackycode, const SymTabT& symtab,
   ErrorHandler& errorHandler) :
  tackycode_(tackycode), symtab_(symtab), errorHandler_(errorHandler)
{}

std::shared_ptr<Asm> AsmGen::gen() {
  std::shared_ptr<Asm> prog = std::visit(*this, *tackycode_);
  instructions_.clear();

  // fill backend symbol table
  for (const auto& it : symtab_) {
    auto sym  = it.second;
    const SymbolAttrs* attrs = sym->getAttrs();
    if (sym->isFunction()) {
      asm_symtab_.addFunction(sym->getName(), attrs->isDefined());
    } else {
      const Type* ty = sym->getType();
      bool isStatic = (attrs->getAttributeType() == SymbolAttrs::STATIC_ATTR);
      asm_symtab_.addObject(sym->getName(), type_to_asm_type(ty), isStatic);
    }
  }

  return replace_pseudo_regs(prog.get());
}

std::shared_ptr<Asm> AsmGen::gen(Tacky* expr) {
  return std::visit(*this, *expr);
}

std::vector<std::shared_ptr<Asm>> AsmGen::gen(
  const std::vector<std::shared_ptr<Tacky>>& exprs) {
  for (auto& expr : exprs) {
    gen(expr.get());
  }
  return {};
}

AsmInstType AsmGen::get_type(std::shared_ptr<Tacky> inst) const {
  const auto visitor = overloads{
      [](const TackyProgram&) { return AsmInstType::ASM_TYPE_ERR; },
      [](const TackyFunction&) { return AsmInstType::ASM_TYPE_ERR; },
      [](const TackyStaticVar&) { return AsmInstType::ASM_TYPE_ERR; },
      [this](const TackyUnary& expr) { return get_type(expr.src); },
      [this](const TackyBinary& expr) { return get_type(expr.src1); },
      [](const TackyConstInt32&) { return AsmInstType::LONG; },
      [](const TackyConstInt64&) { return AsmInstType::QUAD; },
      [this](const TackyVar& var) {
        auto it = symtab_.find(var.identifier);
        const Type* ty = it->second->getType();
        return type_to_asm_type(ty);
      },
      [](const TackyReturn&) { return AsmInstType::ASM_TYPE_ERR; },
      [](const TackyTruncate&) { return AsmInstType::LONG; },
      [](const TackySignExtend&) { return AsmInstType::QUAD; },
      [this](const TackyCopy& copy) { return get_type(copy.src); },
      [](const TackyJump&) { return AsmInstType::ASM_TYPE_ERR; },
      [](const TackyJumpIfZero&) { return AsmInstType::ASM_TYPE_ERR; },
      [](const TackyJumpIfNotZero&) { return AsmInstType::ASM_TYPE_ERR; },
      [](const TackyLabel&) { return AsmInstType::ASM_TYPE_ERR; },
      [this](const TackyFunCall& call) { return get_type(call.dest); },
  };

  return std::visit(visitor, *inst);
}

AsmInstType AsmGen::type_to_asm_type(const Type* ty) const {
  if (ty == BuiltInType::getInt32Ty()) {
    return AsmInstType::LONG;
  } else if (ty == BuiltInType::getInt64Ty()) {
    return AsmInstType::QUAD;
  } else {
    return AsmInstType::ASM_TYPE_ERR;
  }
}

std::shared_ptr<Asm> AsmGen::replace_pseudo_regs(Asm* prog) {
  class ReplacePseudo {
  public:
    ReplacePseudo(const AsmSymTabT& symtab) :
      symtab_(symtab) {}

    std::shared_ptr<Asm> fix(Asm* prog) {
      return std::visit(*this, *prog);
    }

  private:
    const AsmSymTabT& symtab_;
    int fn_stack_size_ = 0;
    std::vector<std::shared_ptr<Asm>> instructions_;
    std::unordered_map<std::string, int> reg_to_offset_;

    std::shared_ptr<Asm> fix_pseudo(Asm* inst) {
      return std::visit(*this, *inst);
    }

  public:
    std::shared_ptr<Asm> operator()(const AsmProgram& prog) {
      std::vector<std::shared_ptr<Asm>> functions;
      for (auto& fn : prog.functions) {
          functions.push_back(fix_pseudo(fn.get()));
      }
      return make_asm<AsmProgram>(functions);
    }

    std::shared_ptr<Asm> operator()(const AsmFunction& fn) {
      fn_stack_size_ = 0;
      instructions_.clear();
      for (auto inst : fn.instructions) {
        auto fix_insts = fix_pseudo(inst.get());
      }

      // stack allocation at the beginning; round up to multiple of 16
      int stack_size = roundUp(fn_stack_size_, 16);
      auto alloc_stack = make_asm<AsmBinary>(
          AsmInstType::QUAD, AsmInst::SUB,
          make_asm<AsmImm>(stack_size),
          make_asm<AsmRegister>(AsmInstType::QUAD, SP));
      instructions_.insert(instructions_.begin(), alloc_stack);
      return make_asm<AsmFunction>(fn.global, fn.name, std::move(instructions_));
    }

    std::shared_ptr<Asm> operator()(const AsmStaticVar& svar) {
      return add_inst<AsmStaticVar>(instructions_, svar.global,
                                               svar.name, svar.alignment, svar.init);
    }

    std::shared_ptr<Asm> operator()(const AsmUnary& unary) {
      // shouldn't be anything other than a single instruction
      auto operand = fix_pseudo(unary.operand.get());
      return add_inst<AsmUnary>(instructions_, unary.type, unary.op, operand);
    }

    std::shared_ptr<Asm> operator()(const AsmBinary& bin) {
      auto operand1 = fix_pseudo(bin.operand1.get());
      auto operand2 = fix_pseudo(bin.operand2.get());
      bool rewrite_inst = false;

      // Instruction rewrite for binary expressions is split into two steps.
      // Multiplication as an example requires that the first operand is smaller
      // than INT_MAX. It also requires that the second operand is not held in
      // memory. It is easier to rewrite it twice than to call fix_pseudo in a
      // loop.
      switch (bin.op) {
        case AsmInst::ADD:
        case AsmInst::SUB:
          // Rewrite addition and subtraction if first operand is larger than
          // INT_MAX or both operands are stored in memory.
          rewrite_inst = (((bin.type == AsmInstType::QUAD) &&
                           isLargerThanInt32(operand1)) ||
                          (isMemoryValue(operand1) && isMemoryValue(operand2)));
          if (rewrite_inst) {
            // movl -4(%rbp), %r10d
            // addl %r10d, -8(%rbp)
            auto reg = make_asm<AsmRegister>(bin.type, AsmReg::R10);
            add_inst<AsmMov>(instructions_, bin.type, operand1, reg);
            return add_inst<AsmBinary>(instructions_, bin.type, bin.op, reg,
                                      operand2);
          }
          break;

        case AsmInst::MUL:
          // Rewrite multiplication if first operand is larger than INT_MAX
          rewrite_inst = ((bin.type == AsmInstType::QUAD) &&
                           isLargerThanInt32(operand1));
          if (rewrite_inst) {
            // movl -4(%rbp), %r10d
            // addl %r10d, -8(%rbp)
            auto reg = make_asm<AsmRegister>(bin.type, AsmReg::R10);
            add_inst<AsmMov>(instructions_, bin.type, operand1, reg);

            if (!isMemoryValue(operand2)) {
              return add_inst<AsmBinary>(instructions_, bin.type, bin.op, reg,
                                        operand2);
            } else {
              // Read first operand from register.
              operand1 = reg;
            }
          }
          break;

        default:
          break;
      }

      switch (bin.op) {
        case AsmInst::MUL:
          if (isMemoryValue(operand2)) {
            // movl -4(%rbp), %r11d
            // imull $3, %r11d
            // movl %r11d, -4(%rbp)
            auto reg = make_asm<AsmRegister>(bin.type, AsmReg::R11);
            add_inst<AsmMov>(instructions_, bin.type, operand2, reg);
            add_inst<AsmBinary>(instructions_, bin.type, bin.op, operand1, reg);
            return add_inst<AsmMov>(instructions_, bin.type, reg, operand2);
          }
          break;

        default:
          break;
      }

      return add_inst<AsmBinary>(instructions_, bin.type, bin.op, operand1,
                                 operand2);
    }

    std::shared_ptr<Asm> operator()(const AsmCmp& cmp) {
      auto operand1 = fix_pseudo(cmp.operand1.get());
      auto operand2 = fix_pseudo(cmp.operand2.get());

      // Rewrite cmp in two steps.
      if (isLargerThanInt32(operand1) ||
          (isMemoryValue(operand1) && isMemoryValue(operand2))) {
        auto reg = make_asm<AsmRegister>(cmp.type, AsmReg::R10);
        add_inst<AsmMov>(instructions_, cmp.type, operand1, reg);
        if (!std::holds_alternative<AsmImm>(*operand2)) {
          return add_inst<AsmCmp>(instructions_, cmp.type, reg, operand2);
        } else {
          operand1 = reg;
        }
      }

      // Rewrite as needed.
      if (std::holds_alternative<AsmImm>(*operand2)) {
        auto reg = make_asm<AsmRegister>(cmp.type, AsmReg::R11);
        add_inst<AsmMov>(instructions_, cmp.type, operand2, reg);
        return add_inst<AsmCmp>(instructions_, cmp.type, operand1, reg);
      } else {
        return add_inst<AsmCmp>(instructions_, cmp.type, operand1, operand2);
      }
    }

    std::shared_ptr<Asm> operator()(const AsmIdiv& idiv) {
      // shouldn't be anything other than a single instruction
      auto operand = fix_pseudo(idiv.operand.get());

      if (std::holds_alternative<AsmImm>(*operand)) {
        // movl $3, %r10d
        // idivl %r10d
        auto reg = make_asm<AsmRegister>(idiv.type, AsmReg::R10);
        add_inst<AsmMov>(instructions_, idiv.type, operand, reg);
        return add_inst<AsmIdiv>(instructions_, idiv.type, reg);
      } else {
        return add_inst<AsmIdiv>(instructions_, idiv.type, operand);
      }
    }

    std::shared_ptr<Asm> operator()(const AsmCdq& cdq) {
      return add_inst<AsmCdq>(instructions_, cdq.type, cdq.dummy);
    }

    std::shared_ptr<Asm> operator()(const AsmJmp& jmp) {
      return add_inst<AsmJmp>(instructions_, jmp.target);
    }

    std::shared_ptr<Asm> operator()(const AsmJmpCC& jmp) {
      return add_inst<AsmJmpCC>(instructions_, jmp.cond_code, jmp.target);
    }

    std::shared_ptr<Asm> operator()(const AsmSetCC& setcc) {
      auto operand = fix_pseudo(setcc.operand.get());
      return add_inst<AsmSetCC>(instructions_, setcc.cond_code, operand);
    }

    std::shared_ptr<Asm> operator()(const AsmLabel& label) {
      return add_inst<AsmLabel>(instructions_, label.identifier);
    }

    std::shared_ptr<Asm> operator()(const AsmMov& mov) {
      auto src = fix_pseudo(mov.src.get());
      auto dest = fix_pseudo(mov.dest.get());

      if ((mov.type == AsmInstType::LONG) && isLargerThanInt32(src)) {
        // Long value is greater than INT32_MAX, truncate first 4 bytes.
        auto val = std::get_if<AsmImm>(src.get());
        int trunc = val->value & 0xFFFFFFFF;
        return add_inst<AsmMov>(instructions_, mov.type,
                                make_asm<AsmImm>(trunc), dest);
      } else if (((mov.type == AsmInstType::QUAD) && isLargerThanInt32(src)) ||
                 (isMemoryValue(src) && isMemoryValue(dest))) {
        auto reg = make_asm<AsmRegister>(mov.type, AsmReg::R10);
        add_inst<AsmMov>(instructions_, mov.type, src, reg);
        return add_inst<AsmMov>(instructions_, mov.type, reg, dest);
      } else {
        return add_inst<AsmMov>(instructions_, mov.type, src, dest);
      }
    }

    std::shared_ptr<Asm> operator()(const AsmMovsx& mov) {
      auto src = fix_pseudo(mov.src.get());
      auto dest = fix_pseudo(mov.dest.get());

      bool isSrcImm = std::get_if<AsmImm>(src.get());
      bool isDstMemory = isMemoryValue(dest);
      if (isSrcImm && isDstMemory) {
        auto reg10 = make_asm<AsmRegister>(AsmInstType::LONG, AsmReg::R10);
        auto reg11 = make_asm<AsmRegister>(AsmInstType::QUAD, AsmReg::R11);
        add_inst<AsmMov>(instructions_, AsmInstType::LONG, src, reg10);
        add_inst<AsmMovsx>(instructions_, reg10, reg11);
        return add_inst<AsmMov>(instructions_, AsmInstType::QUAD, reg11, dest);
      } else if (isSrcImm) {
        auto reg10 = make_asm<AsmRegister>(AsmInstType::LONG, AsmReg::R10);
        add_inst<AsmMov>(instructions_, AsmInstType::LONG, src, reg10);
        return add_inst<AsmMovsx>(instructions_, reg10, dest);
      } else if (isDstMemory) {
        auto reg11 = make_asm<AsmRegister>(AsmInstType::QUAD, AsmReg::R11);
        add_inst<AsmMovsx>(instructions_, src, reg11);
        return add_inst<AsmMov>(instructions_, AsmInstType::QUAD, reg11, dest);
      }

      return add_inst<AsmMovsx>(instructions_, src, dest);
    }

    std::shared_ptr<Asm> operator()(const AsmPush& push) {
      auto operand = fix_pseudo(push.operand.get());
      if (isLargerThanInt32(operand)) {
        auto reg10 = make_asm<AsmRegister>(AsmInstType::QUAD, AsmReg::R10);
        add_inst<AsmMov>(instructions_, AsmInstType::QUAD, operand, reg10);
        return add_inst<AsmPush>(instructions_, reg10);
      }

      return add_inst<AsmPush>(instructions_, operand);
    }

    std::shared_ptr<Asm> operator()(const AsmCall& call) {
      return add_inst<AsmCall>(instructions_, call.fname);
    }

    std::shared_ptr<Asm> operator()(const AsmReturn& ret) {
      return add_inst<AsmReturn>(instructions_, ret.dummy);
    }

    std::shared_ptr<Asm> operator()(const AsmImm& imm) {
      return make_asm<AsmImm>(imm.value);
    }

    std::shared_ptr<Asm> operator()(const AsmRegister& reg) {
      return make_asm<AsmRegister>(reg.type, reg.reg);
    }

    std::shared_ptr<Asm> operator()(const AsmPseudo& pseudo) {
      int stack_offset = 0;
      auto it = reg_to_offset_.find(pseudo.identifier);
      if (it == reg_to_offset_.end()) {
        // Not a stack var. Look in static section.
        auto sym = symtab_.findSymbol(pseudo.identifier);
        assert(sym != nullptr);
        if (sym->isStatic()) {
          return make_asm<AsmData>(pseudo.identifier);
        } else {
          AsmInstType type = sym->getType();
          int alignment = 8;// (type == AsmInstType::LONG) ? 4 : 8;
          int byte_size = 8;//(type == AsmInstType::LONG) ? 4 : 8;
          // align next stack argument to a multiple of 8
          int next_stack_offset = roundUp(fn_stack_size_ + byte_size, alignment);
          // stack locals stored as -ve offset relative to base
          stack_offset = -next_stack_offset;
          fn_stack_size_ = next_stack_offset;
          reg_to_offset_[pseudo.identifier] = stack_offset;
        }
      } else {
        stack_offset = it->second;
      }

      return make_asm<AsmStack>(stack_offset);
    }

    std::shared_ptr<Asm> operator()(const AsmStack& st) {
      return make_asm<AsmStack>(st.offset);
    }

    std::shared_ptr<Asm> operator()(const AsmData& data) {
      return make_asm<AsmData>(data.identifier);
    }
  };

  ReplacePseudo fixed_prog(asm_symtab_);
  return fixed_prog.fix(prog);
}

std::shared_ptr<Asm> AsmGen::operator()(const TackyProgram& prog) {
  std::vector<std::shared_ptr<Asm>> topLevel;
  for (auto& fn : prog.functions) {
    topLevel.push_back(gen(fn.get()));
  }

  for (auto& def : prog.defs) {
    topLevel.push_back(gen(def.get()));
  }

  return make_asm<AsmProgram>(std::move(topLevel));
}

std::shared_ptr<Asm> AsmGen::operator()(const TackyFunction& fn) {
  instructions_.clear();

  // prep params
  int total_params = fn.params.size();
  int num_reg_params = (total_params > static_cast<int>(arg_regs_.size()))
                      ? 6 : total_params;
  int num_stack_args = total_params - num_reg_params;

  // copy params to registers
  for (int i = 0; i < num_reg_params; ++i) {
    auto param = fn.params[i];
    AsmInstType type = get_type(param);
    auto reg = make_asm<AsmRegister>(type, arg_regs_[i]);
    add_inst<AsmMov>(instructions_, type, reg, gen(param.get()));
  }

  // pass arguments on stack
  // stack parameters start at offset +16
  int param_stack_offset = 16;
  for (int i = 0; i < num_stack_args; ++i) {
    int index = num_reg_params + i;
    auto param = fn.params[index];
    add_inst<AsmMov>(instructions_, get_type(param),
                     make_asm<AsmStack>(param_stack_offset + 8 * i),
                     gen(param.get()));
  }

  // instructions
  for (auto& p : fn.instructions) {
    gen(p.get());
  }

  return make_asm<AsmFunction>(fn.global, fn.name, std::move(instructions_));
}

std::shared_ptr<Asm> AsmGen::operator()(const TackyStaticVar& svar) {
  int alignment = 0;
  if (svar.init.getType() == InitialValue::INITIAL_LONG_VALUE) {
    alignment = 8;
  } else if (svar.init.getType() == InitialValue::INITIAL_INT32_VALUE) {
    alignment = 4;
  }

  return make_asm<AsmStaticVar>(svar.global, svar.name, alignment, svar.init);
}

std::shared_ptr<Asm> AsmGen::operator()(const TackyBinary& bin) {
  // src and dest can only be constants or var
  auto src1 = gen(bin.src1.get());
  auto src2 = gen(bin.src2.get());
  auto dest = gen(bin.dest.get());
  AsmInstType type = get_type(bin.src1);

  TokenType optype = bin.op.type;
  if (isRelationalOp(optype)) {
    AsmCondCode cc = AsmCondCode::E;
    switch (optype) {
      case TokenType::EQUAL_EQUAL:
        cc = AsmCondCode::E;
        break;
      case TokenType::BANG_EQUAL:
        cc = AsmCondCode::NE;
        break;
      case TokenType::GREATER:
        cc = AsmCondCode::G;
        break;
      case TokenType::GREATER_EQUAL:
        cc = AsmCondCode::GE;
        break;
      case TokenType::LESS:
        cc = AsmCondCode::L;
        break;
      case TokenType::LESS_EQUAL:
        cc = AsmCondCode::LE;
        break;
      default:
        assert(0);
        break;
    }

    // Cmp(src2, src1)
    // Mov(Imm(0), dst)
    // SetCC(relational_operator, dst)
    add_inst<AsmCmp>(instructions_, type, src2, src1);
    add_inst<AsmMov>(instructions_, type, make_asm<AsmImm>(0), dest);
    return add_inst<AsmSetCC>(instructions_, cc, dest);
  } else if ((optype == TokenType::SLASH) ||
      (optype == TokenType::PERCENT)) {
    // division and remainder
    // Mov(src1, Reg(AX))
    // Cdq
    // Idiv(src2)
    // Mov(Reg(AX) or Reg(DX), dst)
    AsmReg reg = (optype == TokenType::SLASH) ? AsmReg::AX : AsmReg::DX;
    add_inst<AsmMov>(instructions_, type, src1,
                     make_asm<AsmRegister>(type, AsmReg::AX));
    add_inst<AsmCdq>(instructions_, type, 0);
    add_inst<AsmIdiv>(instructions_, type, src2);
    return add_inst<AsmMov>(instructions_, type,
                            make_asm<AsmRegister>(type, reg), dest);
  } else { // everything else
    // Mov(src1, dst)
    // Binary(op, src2, dst)
    add_inst<AsmMov>(instructions_, type, src1, dest);
    AsmInst op = AsmInst::INST_ERR;
    switch (bin.op.type) {
    case TokenType::PLUS:
      op = AsmInst::ADD;
      break;
    case TokenType::MINUS:
      op = AsmInst::SUB;
      break;
    case TokenType::STAR:
      op = AsmInst::MUL;
      break;
    default:
      break;
    }

    return add_inst<AsmBinary>(instructions_, type, op, src2, dest);
  }

  return nullptr;
}

std::shared_ptr<Asm> AsmGen::operator()(const TackyUnary& unary) {
  // src and dest can only be constants or var
  auto src = gen(unary.src.get());
  auto dest = gen(unary.dest.get());
  AsmInstType type = get_type(unary.src);

  if (unary.op.type == TokenType::BANG) {
    add_inst<AsmCmp>(instructions_, type, make_asm<AsmImm>(0), src);
    add_inst<AsmMov>(instructions_, type, make_asm<AsmImm>(0), dest);
    return add_inst<AsmSetCC>(instructions_, AsmCondCode::E, dest);
  } else {
    add_inst<AsmMov>(instructions_, type, src, dest);
    AsmInst op = AsmInst::INST_ERR;
    switch (unary.op.type) {
    case TokenType::TILDE:
      op = AsmInst::NOT;
      break;
    case TokenType::MINUS:
      // For unary expressions, - is lowered to a negation.
      op = AsmInst::NEG;
      break;
    default:
      break;
    }
    return add_inst<AsmUnary>(instructions_, type, op, dest);
  }

  return nullptr;
}

std::shared_ptr<Asm> AsmGen::operator()(const TackyConstInt32& constant) {
  return make_asm<AsmImm>(constant.value);
}

std::shared_ptr<Asm> AsmGen::operator()(const TackyConstInt64& constant) {
  return make_asm<AsmImm>(constant.value);
}

std::shared_ptr<Asm> AsmGen::operator()(const TackyVar& var) {
  return make_asm<AsmPseudo>(var.identifier);
}

std::shared_ptr<Asm> AsmGen::operator()(const TackyReturn& ret) {
  // tacky return can only be constants or var
  auto expr = gen(ret.value.get());
  AsmInstType type = get_type(ret.value);
  add_inst<AsmMov>(instructions_, type, expr,
                   make_asm<AsmRegister>(type, AsmReg::AX));
  return add_inst<AsmReturn>(instructions_, 0);
}

std::shared_ptr<Asm> AsmGen::operator()(const TackyTruncate& trunc) {
  auto src = gen(trunc.src.get());
  auto dest = gen(trunc.dest.get());
  return add_inst<AsmMov>(instructions_, AsmInstType::LONG, src,
                                     dest);
}

std::shared_ptr<Asm> AsmGen::operator()(const TackySignExtend& ext) {
  auto src = gen(ext.src.get());
  auto dest = gen(ext.dest.get());
  return add_inst<AsmMovsx>(instructions_, src, dest);
}

std::shared_ptr<Asm> AsmGen::operator()(const TackyCopy& copy) {
  auto src = gen(copy.src.get());
  auto dest = gen(copy.dest.get());
  return add_inst<AsmMov>(instructions_, get_type(copy.src), src, dest);
}

std::shared_ptr<Asm> AsmGen::get_label(std::shared_ptr<Tacky> inst) {
  auto label = std::get_if<TackyLabel>(inst.get());
  assert(label != nullptr);
  return make_asm<AsmLabel>(label->identifier);
}

// All jump instructions call get_label to generate their argument.
// Calling get() will add an actual label to the program instruction, which is
// not desired.
std::shared_ptr<Asm> AsmGen::operator()(const TackyJump& jmp) {
  auto target = get_label(jmp.target);
  return add_inst<AsmJmp>(instructions_, target);
}

std::shared_ptr<Asm> AsmGen::operator()(const TackyJumpIfZero& jmp) {
  auto cond = gen(jmp.condition.get());
  auto target = get_label(jmp.target);
  AsmInstType type = get_type(jmp.condition);
  add_inst<AsmCmp>(instructions_, type, make_asm<AsmImm>(0), cond);
  return add_inst<AsmJmpCC>(instructions_, AsmCondCode::E, target);
}

std::shared_ptr<Asm> AsmGen::operator()(const TackyJumpIfNotZero& jmp) {
  auto cond = gen(jmp.condition.get());
  auto target = get_label(jmp.target);
  AsmInstType type = get_type(jmp.condition);
  add_inst<AsmCmp>(instructions_, type, make_asm<AsmImm>(0), cond);
  return add_inst<AsmJmpCC>(instructions_, AsmCondCode::NE, target);
}

std::shared_ptr<Asm> AsmGen::operator()(const TackyLabel& label) {
  return add_inst<AsmLabel>(instructions_, label.identifier);
}

std::shared_ptr<Asm> AsmGen::operator()(const TackyFunCall& call) {
  // stack alignment
  int total_args = call.args.size();
  int num_reg_args = (total_args > static_cast<int>(arg_regs_.size()))
                      ? 6 : total_args;
  int num_stack_args = total_args - num_reg_args;
  int stack_padding = 0;
  if ((num_stack_args % 2) != 0) {
    stack_padding = 8;
    // pad by 8 if odd number of stack args
    add_inst<AsmBinary>(instructions_, AsmInstType::QUAD,
                        AsmInst::SUB,
                        make_asm<AsmImm>(stack_padding),
                        make_asm<AsmRegister>(AsmInstType::QUAD, SP));
  }

  // pass arguments in registers
  for (int i = 0; i < num_reg_args; ++i) {
    auto arg = call.args[i];
    AsmInstType type = get_type(arg);
    auto reg = make_asm<AsmRegister>(type, arg_regs_[i]);
    auto res = gen(arg.get());
    add_inst<AsmMov>(instructions_, get_type(arg), res, reg);
  }

  // pass arguments on stack
  for (int i = 0; i < num_stack_args; ++i) {
    int index = total_args - i - 1;
    auto arg = call.args[index];
    auto res = gen(arg.get());
    if (std::get_if<AsmImm>(res.get()) ||
        std::get_if<AsmRegister>(res.get()) ||
        (get_type(arg) == AsmInstType::QUAD)) {
      add_inst<AsmPush>(instructions_, res);
    } else {
      auto reg = make_asm<AsmRegister>(AsmInstType::LONG, AX);
      auto reg_quad = make_asm<AsmRegister>(AsmInstType::QUAD, AX);
      add_inst<AsmMov>(instructions_, AsmInstType::LONG, make_asm<AsmImm>(0),
                       reg);
      add_inst<AsmMov>(instructions_, AsmInstType::LONG, res, reg);
      add_inst<AsmPush>(instructions_, reg_quad);
    }
  }

  // args are setup, call function
  add_inst<AsmCall>(instructions_, call.fname);

  // remove args from stack
  int bytes_dealloc = 8 * num_stack_args + stack_padding;
  if (bytes_dealloc > 0) {
    add_inst<AsmBinary>(instructions_, AsmInstType::QUAD,
                        AsmInst::ADD,
                        make_asm<AsmImm>(bytes_dealloc),
                        make_asm<AsmRegister>(AsmInstType::QUAD, SP));
  }

  // retrieve return value
  AsmInstType return_type = get_type(call.dest);
  auto dest = gen(call.dest.get());
  auto return_reg = make_asm<AsmRegister>(return_type, AsmReg::AX);
  add_inst<AsmMov>(instructions_, return_type, return_reg, dest);
  return dest;
}
