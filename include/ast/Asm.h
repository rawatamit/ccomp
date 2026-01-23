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
class AsmBinary;
class AsmCmp;
class AsmIdiv;
class AsmCdq;
class AsmJmp;
class AsmJmpCC;
class AsmSetCC;
class AsmLabel;
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
  virtual std::any     visitAsmUnary       (std::shared_ptr<AsmUnary       > Asm __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitAsmBinary      (std::shared_ptr<AsmBinary      > Asm __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitAsmCmp         (std::shared_ptr<AsmCmp         > Asm __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitAsmIdiv        (std::shared_ptr<AsmIdiv        > Asm __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitAsmCdq         (std::shared_ptr<AsmCdq         > Asm __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitAsmJmp         (std::shared_ptr<AsmJmp         > Asm __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitAsmJmpCC       (std::shared_ptr<AsmJmpCC       > Asm __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitAsmSetCC       (std::shared_ptr<AsmSetCC       > Asm __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitAsmLabel      (std::shared_ptr<AsmLabel      > Asm __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitAsmMov         (std::shared_ptr<AsmMov         > Asm __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitAsmAllocateStack (std::shared_ptr<AsmAllocateStack > Asm __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitAsmReturn      (std::shared_ptr<AsmReturn      > Asm __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitAsmImm         (std::shared_ptr<AsmImm         > Asm __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitAsmRegister    (std::shared_ptr<AsmRegister    > Asm __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitAsmPseudo      (std::shared_ptr<AsmPseudo      > Asm __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitAsmStack       (std::shared_ptr<AsmStack       > Asm __attribute_maybe_unused__) { return nullptr; }
};

class Asm {
public:
  enum CondCode {
    E,
    NE,
    G,
    GE,
    L,
    LE,
  };
  enum Reg {
    AX,
    DX,
    R10,
    R11,
  };
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

class AsmUnary        : public std::enable_shared_from_this<AsmUnary       >, public Asm { 
public: 
  AsmUnary       (   Token op,    std::shared_ptr<Asm> operand)  :
    op(op), operand(operand) {}
 std::any accept(AsmVisitor& visitor) override {
    std::shared_ptr<AsmUnary       > p{shared_from_this()};
    return visitor.visitAsmUnary       (p);
  }
public: 
   Token op;
   std::shared_ptr<Asm> operand;
};

class AsmBinary       : public std::enable_shared_from_this<AsmBinary      >, public Asm { 
public: 
  AsmBinary      (   Token op,    std::shared_ptr<Asm> operand1,    std::shared_ptr<Asm> operand2)  :
    op(op), operand1(operand1), operand2(operand2) {}
 std::any accept(AsmVisitor& visitor) override {
    std::shared_ptr<AsmBinary      > p{shared_from_this()};
    return visitor.visitAsmBinary      (p);
  }
public: 
   Token op;
   std::shared_ptr<Asm> operand1;
   std::shared_ptr<Asm> operand2;
};

class AsmCmp          : public std::enable_shared_from_this<AsmCmp         >, public Asm { 
public: 
  AsmCmp         (   std::shared_ptr<Asm> operand1,    std::shared_ptr<Asm> operand2)  :
    operand1(operand1), operand2(operand2) {}
 std::any accept(AsmVisitor& visitor) override {
    std::shared_ptr<AsmCmp         > p{shared_from_this()};
    return visitor.visitAsmCmp         (p);
  }
public: 
   std::shared_ptr<Asm> operand1;
   std::shared_ptr<Asm> operand2;
};

class AsmIdiv         : public std::enable_shared_from_this<AsmIdiv        >, public Asm { 
public: 
  AsmIdiv        (   std::shared_ptr<Asm> operand)  :
    operand(operand) {}
 std::any accept(AsmVisitor& visitor) override {
    std::shared_ptr<AsmIdiv        > p{shared_from_this()};
    return visitor.visitAsmIdiv        (p);
  }
public: 
   std::shared_ptr<Asm> operand;
};

class AsmCdq          : public std::enable_shared_from_this<AsmCdq         >, public Asm { 
public: 
  AsmCdq         (   int dummy)  :
    dummy(dummy) {}
 std::any accept(AsmVisitor& visitor) override {
    std::shared_ptr<AsmCdq         > p{shared_from_this()};
    return visitor.visitAsmCdq         (p);
  }
public: 
   int dummy;
};

class AsmJmp          : public std::enable_shared_from_this<AsmJmp         >, public Asm { 
public: 
  AsmJmp         (   std::shared_ptr<AsmLabel> target)  :
    target(target) {}
 std::any accept(AsmVisitor& visitor) override {
    std::shared_ptr<AsmJmp         > p{shared_from_this()};
    return visitor.visitAsmJmp         (p);
  }
public: 
   std::shared_ptr<AsmLabel> target;
};

class AsmJmpCC        : public std::enable_shared_from_this<AsmJmpCC       >, public Asm { 
public: 
  AsmJmpCC       (   Asm::CondCode cond_code,    std::shared_ptr<AsmLabel> target)  :
    cond_code(cond_code), target(target) {}
 std::any accept(AsmVisitor& visitor) override {
    std::shared_ptr<AsmJmpCC       > p{shared_from_this()};
    return visitor.visitAsmJmpCC       (p);
  }
public: 
   Asm::CondCode cond_code;
   std::shared_ptr<AsmLabel> target;
};

class AsmSetCC        : public std::enable_shared_from_this<AsmSetCC       >, public Asm { 
public: 
  AsmSetCC       (   Asm::CondCode cond_code,    std::shared_ptr<Asm> operand)  :
    cond_code(cond_code), operand(operand) {}
 std::any accept(AsmVisitor& visitor) override {
    std::shared_ptr<AsmSetCC       > p{shared_from_this()};
    return visitor.visitAsmSetCC       (p);
  }
public: 
   Asm::CondCode cond_code;
   std::shared_ptr<Asm> operand;
};

class AsmLabel       : public std::enable_shared_from_this<AsmLabel      >, public Asm { 
public: 
  AsmLabel      (   std::string identifier)  :
    identifier(identifier) {}
 std::any accept(AsmVisitor& visitor) override {
    std::shared_ptr<AsmLabel      > p{shared_from_this()};
    return visitor.visitAsmLabel      (p);
  }
public: 
   std::string identifier;
};

class AsmMov          : public std::enable_shared_from_this<AsmMov         >, public Asm { 
public: 
  AsmMov         (   std::shared_ptr<Asm> src,    std::shared_ptr<Asm> dest)  :
    src(src), dest(dest) {}
 std::any accept(AsmVisitor& visitor) override {
    std::shared_ptr<AsmMov         > p{shared_from_this()};
    return visitor.visitAsmMov         (p);
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

class AsmReturn       : public std::enable_shared_from_this<AsmReturn      >, public Asm { 
public: 
  AsmReturn      (   int dummy)  :
    dummy(dummy) {}
 std::any accept(AsmVisitor& visitor) override {
    std::shared_ptr<AsmReturn      > p{shared_from_this()};
    return visitor.visitAsmReturn      (p);
  }
public: 
   int dummy;
};

class AsmImm          : public std::enable_shared_from_this<AsmImm         >, public Asm { 
public: 
  AsmImm         (   int value)  :
    value(value) {}
 std::any accept(AsmVisitor& visitor) override {
    std::shared_ptr<AsmImm         > p{shared_from_this()};
    return visitor.visitAsmImm         (p);
  }
public: 
   int value;
};

class AsmRegister     : public std::enable_shared_from_this<AsmRegister    >, public Asm { 
public: 
  AsmRegister    (   Asm::Reg reg)  :
    reg(reg) {}
 std::any accept(AsmVisitor& visitor) override {
    std::shared_ptr<AsmRegister    > p{shared_from_this()};
    return visitor.visitAsmRegister    (p);
  }
public: 
   Asm::Reg reg;
};

class AsmPseudo       : public std::enable_shared_from_this<AsmPseudo      >, public Asm { 
public: 
  AsmPseudo      (   std::string identifier)  :
    identifier(identifier) {}
 std::any accept(AsmVisitor& visitor) override {
    std::shared_ptr<AsmPseudo      > p{shared_from_this()};
    return visitor.visitAsmPseudo      (p);
  }
public: 
   std::string identifier;
};

class AsmStack        : public std::enable_shared_from_this<AsmStack       >, public Asm { 
public: 
  AsmStack       (   int offset)  :
    offset(offset) {}
 std::any accept(AsmVisitor& visitor) override {
    std::shared_ptr<AsmStack       > p{shared_from_this()};
    return visitor.visitAsmStack       (p);
  }
public: 
   int offset;
};

} // end namespace

#endif
