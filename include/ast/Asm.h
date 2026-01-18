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
class AsmUnary;
class AsmMov;
class AsmAllocateStack;
class AsmReturn;
class AsmImm;
class AsmRegister;
class AsmPseudo;
class AsmStack;
class Expr;

class AsmVisitor {
public:
  virtual ~AsmVisitor() {}
  virtual std::any     visitAsmProgram     (std::shared_ptr<AsmProgram     > Asm __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitAsmFunction    (std::shared_ptr<AsmFunction    > Asm __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitAsmUnary  (std::shared_ptr<AsmUnary  > Asm __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitAsmMov (std::shared_ptr<AsmMov > Asm __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitAsmAllocateStack (std::shared_ptr<AsmAllocateStack > Asm __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitAsmReturn (std::shared_ptr<AsmReturn > Asm __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitAsmImm (std::shared_ptr<AsmImm > Asm __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitAsmRegister (std::shared_ptr<AsmRegister > Asm __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitAsmPseudo (std::shared_ptr<AsmPseudo > Asm __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitAsmStack (std::shared_ptr<AsmStack > Asm __attribute_maybe_unused__) { return nullptr; }
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

class AsmUnary   : public std::enable_shared_from_this<AsmUnary  >, public Asm { 
public: 
  AsmUnary  (   Token op,    std::shared_ptr<Asm> operand)  :
    op(op), operand(operand) {}
 std::any accept(AsmVisitor& visitor) override {
    std::shared_ptr<AsmUnary  > p{shared_from_this()};
    return visitor.visitAsmUnary  (p);
  }
public: 
   Token op;
   std::shared_ptr<Asm> operand;
};

class AsmMov  : public std::enable_shared_from_this<AsmMov >, public Asm { 
public: 
  AsmMov (   std::shared_ptr<Asm> src,    std::shared_ptr<Asm> dest)  :
    src(src), dest(dest) {}
 std::any accept(AsmVisitor& visitor) override {
    std::shared_ptr<AsmMov > p{shared_from_this()};
    return visitor.visitAsmMov (p);
  }
public: 
   std::shared_ptr<Asm> src;
   std::shared_ptr<Asm> dest;
};

class AsmAllocateStack  : public std::enable_shared_from_this<AsmAllocateStack >, public Asm { 
public: 
  AsmAllocateStack (   int size)  :
    size(size) {}
 std::any accept(AsmVisitor& visitor) override {
    std::shared_ptr<AsmAllocateStack > p{shared_from_this()};
    return visitor.visitAsmAllocateStack (p);
  }
public: 
   int size;
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

class AsmImm  : public std::enable_shared_from_this<AsmImm >, public Asm { 
public: 
  AsmImm (   int value)  :
    value(value) {}
 std::any accept(AsmVisitor& visitor) override {
    std::shared_ptr<AsmImm > p{shared_from_this()};
    return visitor.visitAsmImm (p);
  }
public: 
   int value;
};

class AsmRegister  : public std::enable_shared_from_this<AsmRegister >, public Asm { 
public: 
  AsmRegister (   int reg)  :
    reg(reg) {}
 std::any accept(AsmVisitor& visitor) override {
    std::shared_ptr<AsmRegister > p{shared_from_this()};
    return visitor.visitAsmRegister (p);
  }
public: 
   int reg;
};

class AsmPseudo  : public std::enable_shared_from_this<AsmPseudo >, public Asm { 
public: 
  AsmPseudo (   std::string identifier)  :
    identifier(identifier) {}
 std::any accept(AsmVisitor& visitor) override {
    std::shared_ptr<AsmPseudo > p{shared_from_this()};
    return visitor.visitAsmPseudo (p);
  }
public: 
   std::string identifier;
};

class AsmStack  : public std::enable_shared_from_this<AsmStack >, public Asm { 
public: 
  AsmStack (   int offset)  :
    offset(offset) {}
 std::any accept(AsmVisitor& visitor) override {
    std::shared_ptr<AsmStack > p{shared_from_this()};
    return visitor.visitAsmStack (p);
  }
public: 
   int offset;
};

} // end namespace

#endif
