#ifndef Asm_H_
#define Asm_H_

#include "Token.h"
#include <memory>
#include <any>
#include <vector>
namespace ccomp {
class Asm;
class AsmProgram;
class AsmFunction;
class AsmBinaryInst;
class AsmMov;
class AsmImm;
class AsmReturn;
class AsmRegister;
class Expr;

class AsmVisitor {
public:
  virtual ~AsmVisitor() {}
  virtual std::any     visitAsmProgram     (std::shared_ptr<AsmProgram     > Asm) = 0;
  virtual std::any     visitAsmFunction    (std::shared_ptr<AsmFunction    > Asm) = 0;
  virtual std::any     visitAsmBinaryInst  (std::shared_ptr<AsmBinaryInst  > Asm) = 0;
  virtual std::any     visitAsmMov (std::shared_ptr<AsmMov > Asm) = 0;
  virtual std::any     visitAsmImm (std::shared_ptr<AsmImm > Asm) = 0;
  virtual std::any     visitAsmReturn (std::shared_ptr<AsmReturn > Asm) = 0;
  virtual std::any     visitAsmRegister (std::shared_ptr<AsmRegister > Asm) = 0;
};

class Asm {
public:
  virtual ~Asm() {}
  virtual std::any accept(AsmVisitor& visitor) = 0;
};

class AsmProgram      : public std::enable_shared_from_this<AsmProgram     >, public Asm { 
public: 
  AsmProgram     (   std::vector<std::shared_ptr<Asm>> functions)  :
    functions(functions) {}
 std::any accept(AsmVisitor& visitor) override {
    std::shared_ptr<AsmProgram     > p{shared_from_this()};
    return visitor.visitAsmProgram     (p);
  }
public: 
   std::vector<std::shared_ptr<Asm>> functions;
};

class AsmFunction     : public std::enable_shared_from_this<AsmFunction    >, public Asm { 
public: 
  AsmFunction    (   Token name,    std::vector<std::shared_ptr<Asm>> instructions)  :
    name(name), instructions(instructions) {}
 std::any accept(AsmVisitor& visitor) override {
    std::shared_ptr<AsmFunction    > p{shared_from_this()};
    return visitor.visitAsmFunction    (p);
  }
public: 
   Token name;
   std::vector<std::shared_ptr<Asm>> instructions;
};

class AsmBinaryInst   : public std::enable_shared_from_this<AsmBinaryInst  >, public Asm { 
public: 
  AsmBinaryInst  (   Token op,    std::shared_ptr<Asm> src,    std::shared_ptr<Asm> dest)  :
    op(op), src(src), dest(dest) {}
 std::any accept(AsmVisitor& visitor) override {
    std::shared_ptr<AsmBinaryInst  > p{shared_from_this()};
    return visitor.visitAsmBinaryInst  (p);
  }
public: 
   Token op;
   std::shared_ptr<Asm> src;
   std::shared_ptr<Asm> dest;
};

class AsmMov  : public std::enable_shared_from_this<AsmMov >, public Asm { 
public: 
  AsmMov (   std::vector<std::shared_ptr<Asm>> src,    std::shared_ptr<Asm> dest)  :
    src(src), dest(dest) {}
 std::any accept(AsmVisitor& visitor) override {
    std::shared_ptr<AsmMov > p{shared_from_this()};
    return visitor.visitAsmMov (p);
  }
public: 
   std::vector<std::shared_ptr<Asm>> src;
   std::shared_ptr<Asm> dest;
};

class AsmImm  : public std::enable_shared_from_this<AsmImm >, public Asm { 
public: 
  AsmImm (   TokenType type,    std::string operand)  :
    type(type), operand(operand) {}
 std::any accept(AsmVisitor& visitor) override {
    std::shared_ptr<AsmImm > p{shared_from_this()};
    return visitor.visitAsmImm (p);
  }
public: 
   TokenType type;
   std::string operand;
};

class AsmReturn  : public std::enable_shared_from_this<AsmReturn >, public Asm { 
public: 
  AsmReturn (   int dummy)  :
    dummy(dummy) {}
 std::any accept(AsmVisitor& visitor) override {
    std::shared_ptr<AsmReturn > p{shared_from_this()};
    return visitor.visitAsmReturn (p);
  }
public: 
   int dummy;
};

class AsmRegister  : public std::enable_shared_from_this<AsmRegister >, public Asm { 
public: 
  AsmRegister (   int dummy)  :
    dummy(dummy) {}
 std::any accept(AsmVisitor& visitor) override {
    std::shared_ptr<AsmRegister > p{shared_from_this()};
    return visitor.visitAsmRegister (p);
  }
public: 
   int dummy;
};

} // end namespace

#endif
