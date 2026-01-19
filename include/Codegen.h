#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast/Asm.h"
#include "ErrorHandler.h"
#include <any>
#include <memory>

namespace ccomp {
class Codegen : public AsmVisitor {
public:
  Codegen(std::shared_ptr<Asm> program,
          ErrorHandler& errorHandler);
  std::string code();
  virtual ~Codegen() {}

private:
  std::shared_ptr<Asm> program_;
  ErrorHandler& errorHandler_;

  std::string code(std::shared_ptr<Asm> inst);
  std::string code(std::vector<std::shared_ptr<Asm>> insts);

private:
  std::any visitAsmProgram(std::shared_ptr<AsmProgram> Asm) override;
  std::any visitAsmFunction(std::shared_ptr<AsmFunction> Asm) override;
  std::any visitAsmUnary(std::shared_ptr<AsmUnary> Asm) override;
  std::any visitAsmBinary(std::shared_ptr<AsmBinary> bin) override;
  std::any visitAsmIdiv(std::shared_ptr<AsmIdiv> idiv) override;
  std::any visitAsmCdq(std::shared_ptr<AsmCdq> cdq) override;
  std::any visitAsmMov(std::shared_ptr<AsmMov> Asm) override;
  std::any visitAsmAllocateStack(std::shared_ptr<AsmAllocateStack> Asm) override;
  std::any visitAsmReturn(std::shared_ptr<AsmReturn> Asm) override;
  std::any visitAsmImm(std::shared_ptr<AsmImm> Asm) override;
  std::any visitAsmRegister(std::shared_ptr<AsmRegister> Asm) override;
  std::any visitAsmPseudo(std::shared_ptr<AsmPseudo> Asm) override;
  std::any visitAsmStack(std::shared_ptr<AsmStack> Asm) override;
};
}

#endif