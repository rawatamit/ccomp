#ifndef ASMGEN_H
#define ASMGEN_H

#include "ErrorHandler.h"
#include "ast/Asm.h"
#include "ast/Tacky.h"
#include <memory>

namespace ccomp {
class AsmGen : public TackyVisitor {
public:
  AsmGen(std::shared_ptr<Tacky> tackycode, ErrorHandler& errorHandler);
  std::shared_ptr<Asm> gen();

private:
  std::shared_ptr<Tacky> tackycode_;
  ErrorHandler& errorHandler_;

  std::vector<std::shared_ptr<Asm>> gen(std::shared_ptr<Tacky> expr);
  std::vector<std::shared_ptr<Asm>> gen(std::vector<std::shared_ptr<Tacky>> exprs);
  // returns the size in bytes of stack space needed for function
  std::shared_ptr<AsmProgram> replace_pseudo_regs(std::shared_ptr<AsmProgram> fn);

private:
  std::any visitTackyProgram(std::shared_ptr<TackyProgram> Tacky) override;
  std::any visitTackyFunction(std::shared_ptr<TackyFunction> Tacky) override;
  std::any visitTackyUnary(std::shared_ptr<TackyUnary> Tacky) override;
  std::any visitTackyConstant(std::shared_ptr<TackyConstant> Tacky) override;
  std::any visitTackyVar(std::shared_ptr<TackyVar> Tacky) override;
  std::any visitTackyReturn(std::shared_ptr<TackyReturn> Tacky) override;
};
}

#endif // ASMGEN_H
