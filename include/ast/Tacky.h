#ifndef Tacky_H_
#define Tacky_H_

#include "Token.h"
#include <memory>
#include <any>
#include <vector>
namespace ccomp {
class Tacky;
class TackyProgram;
class TackyFunction;
class TackyUnary;
class TackyBinary;
class TackyConstant;
class TackyVar;
class TackyReturn;
class TackyCopy;
class TackyJump;
class TackyJumpIfZero;
class TackyJumpIfNotZero;
class TackyLabel;
class Expr;

class TackyVisitor {
public:
  virtual ~TackyVisitor() {}
  virtual std::any     visitTackyProgram     (std::shared_ptr<TackyProgram     > Tacky __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitTackyFunction    (std::shared_ptr<TackyFunction    > Tacky __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitTackyUnary  (std::shared_ptr<TackyUnary  > Tacky __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitTackyBinary  (std::shared_ptr<TackyBinary  > Tacky __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitTackyConstant (std::shared_ptr<TackyConstant > Tacky __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitTackyVar (std::shared_ptr<TackyVar > Tacky __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitTackyReturn (std::shared_ptr<TackyReturn > Tacky __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitTackyCopy (std::shared_ptr<TackyCopy > Tacky __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitTackyJump (std::shared_ptr<TackyJump > Tacky __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitTackyJumpIfZero (std::shared_ptr<TackyJumpIfZero > Tacky __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitTackyJumpIfNotZero (std::shared_ptr<TackyJumpIfNotZero > Tacky __attribute_maybe_unused__) { return nullptr; }
  virtual std::any     visitTackyLabel (std::shared_ptr<TackyLabel > Tacky __attribute_maybe_unused__) { return nullptr; }
};

class Tacky {
public:
  virtual ~Tacky() {}
  virtual std::any accept(TackyVisitor& visitor) = 0;
};

class TackyProgram      : public std::enable_shared_from_this<TackyProgram     >, public Tacky { 
public: 
  TackyProgram     (   std::vector<std::shared_ptr<Tacky>> functions)  :
    functions(functions) {}
 std::any accept(TackyVisitor& visitor) override {
    std::shared_ptr<TackyProgram     > p{shared_from_this()};
    return visitor.visitTackyProgram     (p);
  }
public: 
   std::vector<std::shared_ptr<Tacky>> functions;
};

class TackyFunction     : public std::enable_shared_from_this<TackyFunction    >, public Tacky { 
public: 
  TackyFunction    (   Token name,    std::vector<std::shared_ptr<Tacky>> instructions)  :
    name(name), instructions(instructions) {}
 std::any accept(TackyVisitor& visitor) override {
    std::shared_ptr<TackyFunction    > p{shared_from_this()};
    return visitor.visitTackyFunction    (p);
  }
public: 
   Token name;
   std::vector<std::shared_ptr<Tacky>> instructions;
};

class TackyUnary   : public std::enable_shared_from_this<TackyUnary  >, public Tacky { 
public: 
  TackyUnary  (   Token op,    std::shared_ptr<Tacky> src,    std::shared_ptr<Tacky> dest)  :
    op(op), src(src), dest(dest) {}
 std::any accept(TackyVisitor& visitor) override {
    std::shared_ptr<TackyUnary  > p{shared_from_this()};
    return visitor.visitTackyUnary  (p);
  }
public: 
   Token op;
   std::shared_ptr<Tacky> src;
   std::shared_ptr<Tacky> dest;
};

class TackyBinary   : public std::enable_shared_from_this<TackyBinary  >, public Tacky { 
public: 
  TackyBinary  (   Token op,    std::shared_ptr<Tacky> src1,    std::shared_ptr<Tacky> src2,    std::shared_ptr<Tacky> dest)  :
    op(op), src1(src1), src2(src2), dest(dest) {}
 std::any accept(TackyVisitor& visitor) override {
    std::shared_ptr<TackyBinary  > p{shared_from_this()};
    return visitor.visitTackyBinary  (p);
  }
public: 
   Token op;
   std::shared_ptr<Tacky> src1;
   std::shared_ptr<Tacky> src2;
   std::shared_ptr<Tacky> dest;
};

class TackyConstant  : public std::enable_shared_from_this<TackyConstant >, public Tacky { 
public: 
  TackyConstant (   int value)  :
    value(value) {}
 std::any accept(TackyVisitor& visitor) override {
    std::shared_ptr<TackyConstant > p{shared_from_this()};
    return visitor.visitTackyConstant (p);
  }
public: 
   int value;
};

class TackyVar  : public std::enable_shared_from_this<TackyVar >, public Tacky { 
public: 
  TackyVar (   std::string identifier)  :
    identifier(identifier) {}
 std::any accept(TackyVisitor& visitor) override {
    std::shared_ptr<TackyVar > p{shared_from_this()};
    return visitor.visitTackyVar (p);
  }
public: 
   std::string identifier;
};

class TackyReturn  : public std::enable_shared_from_this<TackyReturn >, public Tacky { 
public: 
  TackyReturn (   std::shared_ptr<Tacky> value)  :
    value(value) {}
 std::any accept(TackyVisitor& visitor) override {
    std::shared_ptr<TackyReturn > p{shared_from_this()};
    return visitor.visitTackyReturn (p);
  }
public: 
   std::shared_ptr<Tacky> value;
};

class TackyCopy  : public std::enable_shared_from_this<TackyCopy >, public Tacky { 
public: 
  TackyCopy (   std::shared_ptr<Tacky> src,    std::shared_ptr<Tacky> dest)  :
    src(src), dest(dest) {}
 std::any accept(TackyVisitor& visitor) override {
    std::shared_ptr<TackyCopy > p{shared_from_this()};
    return visitor.visitTackyCopy (p);
  }
public: 
   std::shared_ptr<Tacky> src;
   std::shared_ptr<Tacky> dest;
};

class TackyJump  : public std::enable_shared_from_this<TackyJump >, public Tacky { 
public: 
  TackyJump (   std::shared_ptr<TackyLabel> target)  :
    target(target) {}
 std::any accept(TackyVisitor& visitor) override {
    std::shared_ptr<TackyJump > p{shared_from_this()};
    return visitor.visitTackyJump (p);
  }
public: 
   std::shared_ptr<TackyLabel> target;
};

class TackyJumpIfZero  : public std::enable_shared_from_this<TackyJumpIfZero >, public Tacky { 
public: 
  TackyJumpIfZero (   std::shared_ptr<Tacky> condition,    std::shared_ptr<TackyLabel> target)  :
    condition(condition), target(target) {}
 std::any accept(TackyVisitor& visitor) override {
    std::shared_ptr<TackyJumpIfZero > p{shared_from_this()};
    return visitor.visitTackyJumpIfZero (p);
  }
public: 
   std::shared_ptr<Tacky> condition;
   std::shared_ptr<TackyLabel> target;
};

class TackyJumpIfNotZero  : public std::enable_shared_from_this<TackyJumpIfNotZero >, public Tacky { 
public: 
  TackyJumpIfNotZero (   std::shared_ptr<Tacky> condition,    std::shared_ptr<TackyLabel> target)  :
    condition(condition), target(target) {}
 std::any accept(TackyVisitor& visitor) override {
    std::shared_ptr<TackyJumpIfNotZero > p{shared_from_this()};
    return visitor.visitTackyJumpIfNotZero (p);
  }
public: 
   std::shared_ptr<Tacky> condition;
   std::shared_ptr<TackyLabel> target;
};

class TackyLabel  : public std::enable_shared_from_this<TackyLabel >, public Tacky { 
public: 
  TackyLabel (   std::string identifier)  :
    identifier(identifier) {}
 std::any accept(TackyVisitor& visitor) override {
    std::shared_ptr<TackyLabel > p{shared_from_this()};
    return visitor.visitTackyLabel (p);
  }
public: 
   std::string identifier;
};

} // end namespace

#endif
