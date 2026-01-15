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
  std::any visitAsmBinaryInst(std::shared_ptr<AsmBinaryInst> Asm) override;
  std::any visitAsmMov(std::shared_ptr<AsmMov> Asm) override;
  std::any visitAsmImm(std::shared_ptr<AsmImm> Asm) override;
  std::any visitAsmReturn(std::shared_ptr<AsmReturn> Asm) override;
  std::any visitAsmRegister(std::shared_ptr<AsmRegister> Asm) override;
};
}

#endif