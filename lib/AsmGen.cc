#include "AsmGen.h"
#include "ast/Asm.h"
#include "Util.h"
#include <cassert>
#include <memory>
#include <unordered_map>
#include <variant>
#include <vector>

using namespace ccomp;

AsmGen::AsmGen(Tacky* tackycode, ErrorHandler& errorHandler) :
  tackycode_(tackycode), errorHandler_(errorHandler)
{}

std::shared_ptr<Asm> AsmGen::gen() {
  std::shared_ptr<Asm> prog = std::visit(*this, *tackycode_);
  instructions_.clear();
  assert(std::holds_alternative<AsmProgram>(*prog));
  return replace_pseudo_regs(prog.get());
}

std::shared_ptr<Asm> AsmGen::gen(Tacky* expr) {
  return std::visit(*this, *expr);
}

std::vector<std::shared_ptr<Asm>> AsmGen::gen(const std::vector<std::shared_ptr<Tacky>>& exprs) {
  for (auto& expr : exprs) {
    gen(expr.get());
  }
  return {};
}

std::shared_ptr<Asm> AsmGen::replace_pseudo_regs(Asm* prog) {
  class ReplacePseudo {
  public:
    std::shared_ptr<Asm> fix(Asm* prog) {
      return std::visit(*this, *prog);
    }

  private:
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
      // stack allocation should be at the beginning
      auto alloc_stack = make_asm<AsmAllocateStack>(fn_stack_size_);
      instructions_.insert(instructions_.begin(), alloc_stack);
      return make_asm<AsmFunction>(fn.name, std::move(instructions_));
    }

    std::shared_ptr<Asm> operator()(const AsmUnary& unary) {
      // shouldn't be anything other than a single instruction
      auto operand = fix_pseudo(unary.operand.get());
      return make_add_and_return<AsmUnary>(instructions_, unary.op, operand);
    }

    std::shared_ptr<Asm> operator()(const AsmBinary& bin) {
      // shouldn't be anything other than a single instruction
      auto operand1 = fix_pseudo(bin.operand1.get());
      auto operand2 = fix_pseudo(bin.operand2.get());

      switch (bin.op.type) {
        case TokenType::PLUS:
        case TokenType::MINUS:
          if (std::holds_alternative<AsmStack>(*operand1) &&
              std::holds_alternative<AsmStack>(*operand2)) {
            // movl -4(%rbp), %r10d
            // addl %r10d, -8(%rbp)
            auto reg = make_asm<AsmRegister>(AsmReg::R10);
            add_inst<AsmMov>(instructions_, operand1, reg);
            return make_add_and_return<AsmBinary>(instructions_, bin.op, reg, operand2);
          }
          break;

        case TokenType::STAR:
          if (std::holds_alternative<AsmStack>(*operand2)) {
            // movl -4(%rbp), %r11d
            // imull $3, %r11d
            // movl %r11d, -4(%rbp)
            auto reg = make_asm<AsmRegister>(AsmReg::R11);
            add_inst<AsmMov>(instructions_, operand2, reg);
            add_inst<AsmBinary>(instructions_, bin.op, operand1, reg);
            return make_add_and_return<AsmMov>(instructions_, reg, operand2);
          }
        default:
          break;
      }

      return make_add_and_return<AsmBinary>(instructions_, bin.op, operand1, operand2);
    }

    std::shared_ptr<Asm> operator()(const AsmCmp& cmp) {
      auto operand1 = fix_pseudo(cmp.operand1.get());
      auto operand2 = fix_pseudo(cmp.operand2.get());

      if (std::holds_alternative<AsmStack>(*operand1) &&
          std::holds_alternative<AsmStack>(*operand2)) {
        auto reg = make_asm<AsmRegister>(AsmReg::R10);
        add_inst<AsmMov>(instructions_, operand1, reg);
        return make_add_and_return<AsmCmp>(instructions_, reg, operand2);
      } else if (std::holds_alternative<AsmImm>(*operand2)) {
        auto reg = make_asm<AsmRegister>(AsmReg::R11);
        add_inst<AsmMov>(instructions_, operand2, reg);
        return make_add_and_return<AsmCmp>(instructions_, operand1, reg);
      } else {
        return make_add_and_return<AsmCmp>(instructions_, operand1, operand2);
      }
    }

    std::shared_ptr<Asm> operator()(const AsmIdiv& idiv) {
      // shouldn't be anything other than a single instruction
      auto operand = fix_pseudo(idiv.operand.get());

      if (std::holds_alternative<AsmImm>(*operand)) {
        // movl $3, %r10d
        // idivl %r10d
        auto reg = make_asm<AsmRegister>(AsmReg::R10);
        add_inst<AsmMov>(instructions_, operand, reg);
        return make_add_and_return<AsmIdiv>(instructions_, reg);
      } else {
        return make_add_and_return<AsmIdiv>(instructions_, operand);
      }
    }

    std::shared_ptr<Asm> operator()(const AsmCdq& cdq) {
      return make_add_and_return<AsmCdq>(instructions_, cdq.dummy);
    }

    std::shared_ptr<Asm> operator()(const AsmJmp& jmp) {
      return make_add_and_return<AsmJmp>(instructions_, jmp.target);
    }

    std::shared_ptr<Asm> operator()(const AsmJmpCC& jmp) {
      return make_add_and_return<AsmJmpCC>(instructions_, jmp.cond_code, jmp.target);
    }

    std::shared_ptr<Asm> operator()(const AsmSetCC& setcc) {
      auto operand = fix_pseudo(setcc.operand.get());
      return make_add_and_return<AsmSetCC>(instructions_, setcc.cond_code, operand);
    }

    std::shared_ptr<Asm> operator()(const AsmLabel& label) {
      return make_add_and_return<AsmLabel>(instructions_, label.identifier);
    }

    std::shared_ptr<Asm> operator()(const AsmMov& mov) {
      auto src = fix_pseudo(mov.src.get());
      auto dest = fix_pseudo(mov.dest.get());

      if (std::holds_alternative<AsmStack>(*src) &&
          std::holds_alternative<AsmStack>(*dest)) {
        auto reg = make_asm<AsmRegister>(AsmReg::R10);
        add_inst<AsmMov>(instructions_, src, reg);
        return make_add_and_return<AsmMov>(instructions_, reg, dest);
      } else {
        return make_add_and_return<AsmMov>(instructions_, src, dest);
      }
    }

    std::shared_ptr<Asm> operator()(const AsmAllocateStack&) {
      assert(0);
      return nullptr;
    }

    std::shared_ptr<Asm> operator()(const AsmReturn& ret) {
      return make_add_and_return<AsmReturn>(instructions_, ret.dummy);
    }

    std::shared_ptr<Asm> operator()(const AsmImm& imm) {
      return make_asm<AsmImm>(imm.value);
    }

    std::shared_ptr<Asm> operator()(const AsmRegister& reg) {
      return make_asm<AsmRegister>(reg.reg);
    }

    std::shared_ptr<Asm> operator()(const AsmPseudo& pseudo) {
      // TODO: only integers for now
      int stack_offset = INT_MIN;
      auto it = reg_to_offset_.find(pseudo.identifier);
      if (it == reg_to_offset_.end()) {
        fn_stack_size_ += 4;
        stack_offset = fn_stack_size_;
        reg_to_offset_[pseudo.identifier] = stack_offset;
      } else {
        stack_offset = it->second;
      }

      return make_asm<AsmStack>(stack_offset);
    }

    std::shared_ptr<Asm> operator()(const AsmStack&) {
      assert(0);
      return nullptr;
    }
  };

  ReplacePseudo fixed_prog;
  return fixed_prog.fix(prog);
}

std::shared_ptr<Asm> AsmGen::operator()(const TackyProgram& prog) {
  std::vector<std::shared_ptr<Asm>> fns;
  for (auto& fn : prog.functions) {
    fns.push_back(gen(fn.get()));
  }
  return make_asm<AsmProgram>(std::move(fns));
}

std::shared_ptr<Asm> AsmGen::operator()(const TackyFunction& fn) {
  instructions_.clear();
  for (auto& p : fn.instructions) {
    gen(p.get());
  }
  return make_asm<AsmFunction>(fn.name, std::move(instructions_));
}

std::shared_ptr<Asm> AsmGen::operator()(const TackyBinary& bin) {
  // src and dest can only be constants or var
  auto src1 = gen(bin.src1.get());
  auto src2 = gen(bin.src2.get());
  auto dest = gen(bin.dest.get());

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
    add_inst<AsmCmp>(instructions_, src2, src1);
    add_inst<AsmMov>(instructions_, make_asm<AsmImm>(0), dest);
    return make_add_and_return<AsmSetCC>(instructions_, cc, dest);
  } else if ((optype == TokenType::SLASH) ||
      (optype == TokenType::PERCENT)) {
    // division and remainder
    // Mov(src1, Reg(AX))
    // Cdq
    // Idiv(src2)
    // Mov(Reg(AX) or Reg(DX), dst)
    AsmReg reg = (optype == TokenType::SLASH) ? AsmReg::AX : AsmReg::DX;
    add_inst<AsmMov>(instructions_, src1, make_asm<AsmRegister>(AsmReg::AX));
    add_inst<AsmCdq>(instructions_, 0);
    add_inst<AsmIdiv>(instructions_, src2);
    return make_add_and_return<AsmMov>(instructions_, make_asm<AsmRegister>(reg), dest);
  } else { // everything else
    // Mov(src1, dst)
    // Binary(op, src2, dst)
    add_inst<AsmMov>(instructions_, src1, dest);
    return make_add_and_return<AsmBinary>(instructions_, bin.op, src2, dest);
  }

  return nullptr;
}

std::shared_ptr<Asm> AsmGen::operator()(const TackyUnary& unary) {
  // src and dest can only be constants or var
  auto src = gen(unary.src.get());
  auto dest = gen(unary.dest.get());

  if (unary.op.type == TokenType::BANG) {
    add_inst<AsmCmp>(instructions_, make_asm<AsmImm>(0), src);
    add_inst<AsmMov>(instructions_, make_asm<AsmImm>(0), dest);
    return make_add_and_return<AsmSetCC>(instructions_, AsmCondCode::E, dest);
  } else {
    add_inst<AsmMov>(instructions_, src, dest);
    return make_add_and_return<AsmUnary>(instructions_, unary.op, dest);
  }

  return nullptr;
}

std::shared_ptr<Asm> AsmGen::operator()(const TackyConstant& constant) {
  return make_asm<AsmImm>(constant.value);
}

std::shared_ptr<Asm> AsmGen::operator()(const TackyVar& var) {
  return make_asm<AsmPseudo>(var.identifier);
}

std::shared_ptr<Asm> AsmGen::operator()(const TackyReturn& ret) {
  // tacky return can only be constants or var
  auto expr = gen(ret.value.get());
  add_inst<AsmMov>(instructions_, expr, make_asm<AsmRegister>(AsmReg::AX));
  return make_add_and_return<AsmReturn>(instructions_, 0);
}

std::shared_ptr<Asm> AsmGen::operator()(const TackyCopy& copy) {
  auto src = gen(copy.src.get());
  auto dest = gen(copy.dest.get());
  return make_add_and_return<AsmMov>(instructions_, src, dest);
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
  return make_add_and_return<AsmJmp>(instructions_, target);
}

std::shared_ptr<Asm> AsmGen::operator()(const TackyJumpIfZero& jmp) {
  auto cond = gen(jmp.condition.get());
  auto target = get_label(jmp.target);
  add_inst<AsmCmp>(instructions_, make_asm<AsmImm>(0), cond);
  return make_add_and_return<AsmJmpCC>(instructions_, AsmCondCode::E, target);
}

std::shared_ptr<Asm> AsmGen::operator()(const TackyJumpIfNotZero& jmp) {
  auto cond = gen(jmp.condition.get());
  auto target = get_label(jmp.target);
  add_inst<AsmCmp>(instructions_, make_asm<AsmImm>(0), cond);
  return make_add_and_return<AsmJmpCC>(instructions_, AsmCondCode::NE, target);
}

std::shared_ptr<Asm> AsmGen::operator()(const TackyLabel& label) {
  return make_add_and_return<AsmLabel>(instructions_, label.identifier);
}
