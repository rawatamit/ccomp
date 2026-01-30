#ifndef Tacky_H_
#define Tacky_H_

#include "Token.h"
#include <memory>
#include <string>
#include <vector>
#include <variant>

namespace ccomp {
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
using Tacky = std::variant<TackyProgram, TackyFunction, TackyUnary, TackyBinary, TackyConstant, TackyVar, TackyReturn, TackyCopy, TackyJump, TackyJumpIfZero, TackyJumpIfNotZero, TackyLabel>;
class TackyProgram {
public: 
  TackyProgram(  std::vector<std::shared_ptr<Tacky>> functions) :
    functions(functions) {}
public: 
  std::vector<std::shared_ptr<Tacky>> functions;
};

class TackyFunction {
public: 
  TackyFunction(  Token name,   std::vector<std::shared_ptr<Tacky>> instructions) :
    name(name), instructions(instructions) {}
public: 
  Token name;
  std::vector<std::shared_ptr<Tacky>> instructions;
};

class TackyUnary {
public: 
  TackyUnary(  Token op,   std::shared_ptr<Tacky> src,   std::shared_ptr<Tacky> dest) :
    op(op), src(src), dest(dest) {}
public: 
  Token op;
  std::shared_ptr<Tacky> src;
  std::shared_ptr<Tacky> dest;
};

class TackyBinary {
public: 
  TackyBinary(  Token op,   std::shared_ptr<Tacky> src1,   std::shared_ptr<Tacky> src2,   std::shared_ptr<Tacky> dest) :
    op(op), src1(src1), src2(src2), dest(dest) {}
public: 
  Token op;
  std::shared_ptr<Tacky> src1;
  std::shared_ptr<Tacky> src2;
  std::shared_ptr<Tacky> dest;
};

class TackyConstant {
public: 
  TackyConstant(  int value) :
    value(value) {}
public: 
  int value;
};

class TackyVar {
public: 
  TackyVar(  std::string identifier) :
    identifier(identifier) {}
public: 
  std::string identifier;
};

class TackyReturn {
public: 
  TackyReturn(  std::shared_ptr<Tacky> value) :
    value(value) {}
public: 
  std::shared_ptr<Tacky> value;
};

class TackyCopy {
public: 
  TackyCopy(  std::shared_ptr<Tacky> src,   std::shared_ptr<Tacky> dest) :
    src(src), dest(dest) {}
public: 
  std::shared_ptr<Tacky> src;
  std::shared_ptr<Tacky> dest;
};

class TackyJump {
public: 
  TackyJump(  std::shared_ptr<Tacky> target) :
    target(target) {}
public: 
  std::shared_ptr<Tacky> target;
};

class TackyJumpIfZero {
public: 
  TackyJumpIfZero(  std::shared_ptr<Tacky> condition,   std::shared_ptr<Tacky> target) :
    condition(condition), target(target) {}
public: 
  std::shared_ptr<Tacky> condition;
  std::shared_ptr<Tacky> target;
};

class TackyJumpIfNotZero {
public: 
  TackyJumpIfNotZero(  std::shared_ptr<Tacky> condition,   std::shared_ptr<Tacky> target) :
    condition(condition), target(target) {}
public: 
  std::shared_ptr<Tacky> condition;
  std::shared_ptr<Tacky> target;
};

class TackyLabel {
public: 
  TackyLabel(  std::string identifier) :
    identifier(identifier) {}
public: 
  std::string identifier;
};

} // end namespace

#endif
