#ifndef ASMGEN_H
#define ASMGEN_H

#include "ErrorHandler.h"
#include "ast/Asm.h"
#include "ast/Tacky.h"
#include <memory>

namespace ccomp {
class AsmGen {
public:
  AsmGen(Tacky* tackycode, ErrorHandler& errorHandler);
  std::shared_ptr<Asm> gen();

private:
  Tacky* tackycode_;
  ErrorHandler& errorHandler_;
  std::vector<std::shared_ptr<Asm>> instructions_;

  std::shared_ptr<Asm> gen(Tacky* expr);
  std::vector<std::shared_ptr<Asm>> gen(const std::vector<std::shared_ptr<Tacky>>& exprs);
  std::shared_ptr<Asm> get_label(std::shared_ptr<Tacky> inst);

  // returns the size in bytes of stack space needed for function
  std::shared_ptr<Asm> replace_pseudo_regs(Asm* fn);

public:
  std::shared_ptr<Asm> operator()(const TackyProgram& Tacky);
  std::shared_ptr<Asm> operator()(const TackyFunction& Tacky);
  std::shared_ptr<Asm> operator()(const TackyUnary& Tacky);
  std::shared_ptr<Asm> operator()(const TackyBinary& Tacky);
  std::shared_ptr<Asm> operator()(const TackyConstant& Tacky);
  std::shared_ptr<Asm> operator()(const TackyVar& Tacky);
  std::shared_ptr<Asm> operator()(const TackyReturn& Tacky);
  std::shared_ptr<Asm> operator()(const TackyCopy& copy);
  std::shared_ptr<Asm> operator()(const TackyJump& jmp);
  std::shared_ptr<Asm> operator()(const TackyJumpIfZero& jmp);
  std::shared_ptr<Asm> operator()(const TackyJumpIfNotZero& jmp);
  std::shared_ptr<Asm> operator()(const TackyLabel& label);
};
}

#endif // ASMGEN_H
