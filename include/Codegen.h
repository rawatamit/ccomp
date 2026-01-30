#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast/Asm.h"
#include "ErrorHandler.h"
#include <memory>

namespace ccomp {
class Codegen {
public:
  Codegen(const Asm* program,
          ErrorHandler& errorHandler);
  std::string code();
  virtual ~Codegen() {}

private:
  const Asm* program_;
  ErrorHandler& errorHandler_;

  std::string code(std::shared_ptr<Asm> inst);
  std::string code(std::vector<std::shared_ptr<Asm>> insts);

public:
  std::string operator()(const AsmProgram& Asm);
  std::string operator()(const AsmFunction& Asm);
  std::string operator()(const AsmUnary& Asm);
  std::string operator()(const AsmBinary& bin);
  std::string operator()(const AsmCmp& cmp);
  std::string operator()(const AsmIdiv& idiv);
  std::string operator()(const AsmCdq& cdq);
  std::string operator()(const AsmJmp& jmp);
  std::string operator()(const AsmJmpCC& jmpcc);
  std::string operator()(const AsmSetCC& setcc);
  std::string operator()(const AsmLabel& label);
  std::string operator()(const AsmMov& Asm);
  std::string operator()(const AsmAllocateStack& Asm);
  std::string operator()(const AsmReturn& Asm);
  std::string operator()(const AsmImm& Asm);
  std::string operator()(const AsmRegister& Asm);
  std::string operator()(const AsmPseudo& Asm);
  std::string operator()(const AsmStack& Asm);
};
}

#endif