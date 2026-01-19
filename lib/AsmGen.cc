#include "AsmGen.h"
#include "ast/Asm.h"
#include <algorithm>
#include <any>
#include <cassert>
#include <iterator>
#include <memory>
#include <unordered_map>
#include <vector>

using namespace ccomp;

AsmGen::AsmGen(std::shared_ptr<Tacky> tackycode, ErrorHandler& errorHandler) :
  tackycode_(tackycode), errorHandler_(errorHandler)
{}

std::shared_ptr<Asm> AsmGen::gen() {
  auto prog = std::any_cast<std::shared_ptr<AsmProgram>>(tackycode_->accept(*this));
  prog = replace_pseudo_regs(prog);
  return prog;
}

std::vector<std::shared_ptr<Asm>> AsmGen::gen(std::shared_ptr<Tacky> expr) {
  auto val = expr->accept(*this);
  return std::any_cast<std::vector<std::shared_ptr<Asm>>>(val);
}

std::vector<std::shared_ptr<Asm>> AsmGen::gen(std::vector<std::shared_ptr<Tacky>> exprs) {
  std::vector<std::shared_ptr<Asm>> genexprs;
  for (auto expr : exprs) {
    auto genexpr = gen(expr);
    std::copy(genexpr.begin(), genexpr.end(), std::back_inserter(genexprs));
  }
  return genexprs;
}

std::shared_ptr<AsmProgram> AsmGen::replace_pseudo_regs(std::shared_ptr<AsmProgram> prog) {
  class ReplacePseudo : public AsmVisitor {
  public:
    std::shared_ptr<AsmProgram> fix(std::shared_ptr<AsmProgram> prog) {
      return std::any_cast<std::shared_ptr<AsmProgram>>(visitAsmProgram(prog));
    }

  private:
    int fn_stack_size_ = 0;
    std::unordered_map<std::string, int> reg_to_offset_;

    std::vector<std::shared_ptr<Asm>> fix_pseudo(std::shared_ptr<Asm> inst) {
      return std::any_cast<std::vector<std::shared_ptr<Asm>>>(
                inst->accept(*this));
    }

    std::any visitAsmProgram(std::shared_ptr<AsmProgram> prog) {
      std::vector<std::shared_ptr<Asm>> functions;
      for (auto fn : prog->functions) {
          functions.push_back(
            std::any_cast<std::shared_ptr<AsmFunction>>(fn->accept(*this)));
      }
      return std::make_shared<AsmProgram>(functions);
    }

    std::any visitAsmFunction(std::shared_ptr<AsmFunction> fn) {
      fn_stack_size_ = 0;
      std::vector<std::shared_ptr<Asm>> insts;
      for (auto inst : fn->instructions) {
        auto fix_insts = fix_pseudo(inst);
        std::copy(fix_insts.begin(), fix_insts.end(), std::back_inserter(insts));
      }
      insts.emplace(insts.begin(),
                    std::make_shared<AsmAllocateStack>(fn_stack_size_));
      return std::make_shared<AsmFunction>(fn->name, insts);
    }

    std::any visitAsmUnary(std::shared_ptr<AsmUnary> unary) {
      // shouldn't be anything other than a single instruction
      auto operand = fix_pseudo(unary->operand).back();
      return std::vector<std::shared_ptr<Asm>>{
        std::make_shared<AsmUnary>(unary->op, operand)};
    }

    std::any visitAsmBinary(std::shared_ptr<AsmBinary> bin) {
      // shouldn't be anything other than a single instruction
      auto operand1 = fix_pseudo(bin->operand1).back();
      auto operand2 = fix_pseudo(bin->operand2).back();

      switch (bin->op.type) {
        case TokenType::PLUS:
        case TokenType::MINUS:
          if (std::dynamic_pointer_cast<AsmStack>(operand1) &&
              std::dynamic_pointer_cast<AsmStack>(operand2)) {
            // movl -4(%rbp), %r10d
            // addl %r10d, -8(%rbp)
            auto reg = std::make_shared<AsmRegister>(10);
            return std::vector<std::shared_ptr<Asm>>{
              std::make_shared<AsmMov>(operand1, reg),
              std::make_shared<AsmBinary>(bin->op, reg, operand2)};
          }
          break;

        case TokenType::STAR:
          if (std::dynamic_pointer_cast<AsmStack>(operand2)) {
            // movl -4(%rbp), %r11d
            // imull $3, %r11d
            // movl %r11d, -4(%rbp)
            auto reg = std::make_shared<AsmRegister>(11);
            return std::vector<std::shared_ptr<Asm>>{
              std::make_shared<AsmMov>(operand2, reg),
              std::make_shared<AsmBinary>(bin->op, operand1, reg),
              std::make_shared<AsmMov>(reg, operand2)};
          }
        default:
          break;
      }

      return std::vector<std::shared_ptr<Asm>>{
        std::make_shared<AsmBinary>(bin->op, operand1, operand2)};
    }

    std::any visitAsmIdiv(std::shared_ptr<AsmIdiv> idiv) {
      // shouldn't be anything other than a single instruction
      auto operand = fix_pseudo(idiv->operand).back();

      if (auto imm = std::dynamic_pointer_cast<AsmImm>(operand)) {
        auto reg = std::make_shared<AsmRegister>(10);
        return std::vector<std::shared_ptr<Asm>>{
          // movl $3, %r10d
          // idivl %r10d
          std::make_shared<AsmMov>(imm, reg),
          std::make_shared<AsmIdiv>(reg)};
      } else {
        return std::vector<std::shared_ptr<Asm>>{
          std::make_shared<AsmIdiv>(operand)};
      }
    }

    std::any visitAsmCdq(std::shared_ptr<AsmCdq> cdq) {
      return std::vector<std::shared_ptr<Asm>>{cdq};
    }

    std::any visitAsmMov(std::shared_ptr<AsmMov> mov) {
      auto src = fix_pseudo(mov->src).back();
      auto dest = fix_pseudo(mov->dest).back();

      if (std::dynamic_pointer_cast<AsmStack>(src) &&
          std::dynamic_pointer_cast<AsmStack>(dest)) {
        auto reg = std::make_shared<AsmRegister>(10);
        return std::vector<std::shared_ptr<Asm>>{
          std::make_shared<AsmMov>(src, reg),
          std::make_shared<AsmMov>(reg, dest)};
      }

      return std::vector<std::shared_ptr<Asm>>{
        std::make_shared<AsmMov>(src, dest)};
    }

    std::any visitAsmAllocateStack(std::shared_ptr<AsmAllocateStack>) {
      assert(0);
      return nullptr;
    }

    std::any visitAsmReturn(std::shared_ptr<AsmReturn> ret) {
      return std::vector<std::shared_ptr<Asm>>{ret};
    }

    std::any visitAsmImm(std::shared_ptr<AsmImm> imm) {
      return std::vector<std::shared_ptr<Asm>>{imm};
    }

    std::any visitAsmRegister(std::shared_ptr<AsmRegister> reg) {
      return std::vector<std::shared_ptr<Asm>>{reg};
    }

    std::any visitAsmPseudo(std::shared_ptr<AsmPseudo> pseudo) {
      // TODO: only integers for now
      int stack_offset = INT_MIN;
      auto it = reg_to_offset_.find(pseudo->identifier);
      if (it == reg_to_offset_.end()) {
        fn_stack_size_ += 4;
        stack_offset = fn_stack_size_;
        reg_to_offset_[pseudo->identifier] = stack_offset;
      } else {
        stack_offset = it->second;
      }

      return std::vector<std::shared_ptr<Asm>>{
        std::make_shared<AsmStack>(stack_offset)};
    }

    std::any visitAsmStack(std::shared_ptr<AsmStack>) {
      assert(0);
      return nullptr;
    }
  };

  ReplacePseudo repl_pseudo;
  return repl_pseudo.fix(prog);
}

std::any AsmGen::visitTackyProgram(std::shared_ptr<TackyProgram> prog) {
  std::vector<std::shared_ptr<Asm>> fns;
  for (auto fn : prog->functions) {
    auto fnp = std::any_cast<std::shared_ptr<AsmFunction>>(fn->accept(*this));
    fns.push_back(fnp);
  }
  return std::make_shared<AsmProgram>(fns);
}

std::any AsmGen::visitTackyFunction(std::shared_ptr<TackyFunction> fn) {
  std::vector<std::shared_ptr<Asm>> insts;
  for (auto p : fn->instructions) {
    auto geninsts = gen(p);
    std::copy(geninsts.begin(), geninsts.end(), std::back_inserter(insts));
  }

  return std::make_shared<AsmFunction>(fn->name, insts);
}

std::any AsmGen::visitTackyBinary(std::shared_ptr<TackyBinary> bin) {
  // src and dest can only be constants or var
  auto src1 = gen(bin->src1).back();
  auto src2 = gen(bin->src2).back();
  auto dest = gen(bin->dest).back();

  // division and remainder
  TokenType optype = bin->op.type;
  if ((optype == TokenType::SLASH) ||
      (optype == TokenType::PERCENT)) {
    // Mov(src1, Reg(AX))
    // Cdq
    // Idiv(src2)
    // Mov(Reg(AX) or Reg(DX), dst)
    int reg = (optype == TokenType::SLASH) ? 0 : 3;
    return std::vector<std::shared_ptr<Asm>>{
      std::make_shared<AsmMov>(src1, std::make_shared<AsmRegister>(0)),
      std::make_shared<AsmCdq>(0),
      std::make_shared<AsmIdiv>(src2),
      std::make_shared<AsmMov>(std::make_shared<AsmRegister>(reg), dest)};
  } else { // everything else
    // Mov(src1, dst)
    // Binary(op, src2, dst)
    return std::vector<std::shared_ptr<Asm>>{
      std::make_shared<AsmMov>(src1, dest),
      std::make_shared<AsmBinary>(bin->op, src2, dest)};
  }
}

std::any AsmGen::visitTackyUnary(std::shared_ptr<TackyUnary> unary) {
  // src and dest can only be constants or var
  auto src = gen(unary->src).back();
  auto dest = gen(unary->dest).back();

  return std::vector<std::shared_ptr<Asm>>{
    std::make_shared<AsmMov>(src, dest),
    std::make_shared<AsmUnary>(unary->op, dest)};
}

std::any AsmGen::visitTackyConstant(std::shared_ptr<TackyConstant> constant) {
  return std::vector<std::shared_ptr<Asm>>({
    std::make_shared<AsmImm>(constant->value)});
}

std::any AsmGen::visitTackyVar(std::shared_ptr<TackyVar> var) {
  return std::vector<std::shared_ptr<Asm>>({
    std::make_shared<AsmPseudo>(var->identifier)});
}

std::any AsmGen::visitTackyReturn(std::shared_ptr<TackyReturn> ret) {
  // tacky return can only be constants or var
  auto expr = gen(ret->value).back();
  return std::vector<std::shared_ptr<Asm>>({
    std::make_shared<AsmMov>(expr, std::make_shared<AsmRegister>(0)),
    std::make_shared<AsmReturn>(0)});
}
