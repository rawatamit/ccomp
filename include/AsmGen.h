#ifndef ASMGEN_H
#define ASMGEN_H

#include "ErrorHandler.h"
#include "SymbolTable.h"
#include "AsmSymbolTable.h"
#include "ast/Asm.h"
#include "ast/Tacky.h"
#include <memory>

namespace ccomp {
class AsmGen {
public:
  AsmGen(Tacky* tackycode, const SymTabT& symtab,
         ErrorHandler& errorHandler);
  std::shared_ptr<Asm> gen();

private:
  Tacky* tackycode_;
  const SymTabT& symtab_;
  ErrorHandler& errorHandler_;
  std::vector<std::shared_ptr<Asm>> instructions_;
  AsmSymTabT asm_symtab_;
  // arguments passed in registers
  static const std::vector<AsmReg> arg_regs_;

  std::shared_ptr<Asm> gen(Tacky* expr);
  std::vector<std::shared_ptr<Asm>> gen(const std::vector<std::shared_ptr<Tacky>>& exprs);
  std::shared_ptr<Asm> get_label(std::shared_ptr<Tacky> inst);
  const Type* get_type(std::shared_ptr<Tacky> inst) const;
  AsmInstType type_to_asm_type(const Type* ty) const;

  // returns the size in bytes of stack space needed for function
  std::shared_ptr<Asm> replace_pseudo_regs(Asm* fn);

public:
  std::shared_ptr<Asm> operator()(const TackyProgram& Tacky);
  std::shared_ptr<Asm> operator()(const TackyFunction& Tacky);
  std::shared_ptr<Asm> operator()(const TackyStaticVar& Tacky);
  std::shared_ptr<Asm> operator()(const TackyUnary& Tacky);
  std::shared_ptr<Asm> operator()(const TackyBinary& Tacky);
  std::shared_ptr<Asm> operator()(const TackyConstInt32& Tacky);
  std::shared_ptr<Asm> operator()(const TackyConstUInt32& Tacky);
  std::shared_ptr<Asm> operator()(const TackyConstInt64& Tacky);
  std::shared_ptr<Asm> operator()(const TackyConstUInt64& Tacky);
  std::shared_ptr<Asm> operator()(const TackyVar& Tacky);
  std::shared_ptr<Asm> operator()(const TackyReturn& Tacky);
  std::shared_ptr<Asm> operator()(const TackyTruncate&);
  std::shared_ptr<Asm> operator()(const TackySignExtend&);
  std::shared_ptr<Asm> operator()(const TackyZeroExtend&);
  std::shared_ptr<Asm> operator()(const TackyCopy& copy);
  std::shared_ptr<Asm> operator()(const TackyJump& jmp);
  std::shared_ptr<Asm> operator()(const TackyJumpIfZero& jmp);
  std::shared_ptr<Asm> operator()(const TackyJumpIfNotZero& jmp);
  std::shared_ptr<Asm> operator()(const TackyLabel& label);
  std::shared_ptr<Asm> operator()(const TackyFunCall& call);
};
}

#endif // ASMGEN_H
