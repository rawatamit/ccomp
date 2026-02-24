#ifndef Tacky_H_
#define Tacky_H_

#include "Token.h"
#include <memory>
#include <string>
#include <vector>
#include <variant>

namespace ccomp {
class Type;
class TackyProgram;
class TackyFunction;
class TackyStaticVar;
class TackyUnary;
class TackyBinary;
class TackyConstInt32;
class TackyConstInt64;
class TackyVar;
class TackyReturn;
class TackyTruncate;
class TackySignExtend;
class TackyCopy;
class TackyJump;
class TackyJumpIfZero;
class TackyJumpIfNotZero;
class TackyLabel;
class TackyFunCall;
typedef const Type* type_ptr;
using Tacky = std::variant<TackyProgram, TackyFunction, TackyStaticVar, TackyUnary, TackyBinary, TackyConstInt32, TackyConstInt64, TackyVar, TackyReturn, TackyTruncate, TackySignExtend, TackyCopy, TackyJump, TackyJumpIfZero, TackyJumpIfNotZero, TackyLabel, TackyFunCall>;
class TackyProgram {
public: 
  TackyProgram(std::vector<std::shared_ptr<Tacky>> functions, std::vector<std::shared_ptr<Tacky>> defs) :
    functions(std::move(functions)), defs(std::move(defs)) {}
public: 
  std::vector<std::shared_ptr<Tacky>> functions;
  std::vector<std::shared_ptr<Tacky>> defs;
};

class TackyFunction {
public: 
  TackyFunction(bool global, std::string name, std::vector<std::shared_ptr<Tacky>> params, std::vector<std::shared_ptr<Tacky>> instructions) :
    global(global), name(name), params(std::move(params)), instructions(std::move(instructions)) {}
public: 
  bool global;
  std::string name;
  std::vector<std::shared_ptr<Tacky>> params;
  std::vector<std::shared_ptr<Tacky>> instructions;
};

class TackyStaticVar {
public: 
  TackyStaticVar(bool global, std::string name, InitialValue init) :
    global(global), name(name), init(init) {}
public: 
  bool global;
  std::string name;
  InitialValue init;
};

class TackyUnary {
public: 
  TackyUnary(Token op, std::shared_ptr<Tacky> src, std::shared_ptr<Tacky> dest) :
    op(op), src(src), dest(dest) {}
public: 
  Token op;
  std::shared_ptr<Tacky> src;
  std::shared_ptr<Tacky> dest;
};

class TackyBinary {
public: 
  TackyBinary(Token op, std::shared_ptr<Tacky> src1, std::shared_ptr<Tacky> src2, std::shared_ptr<Tacky> dest) :
    op(op), src1(src1), src2(src2), dest(dest) {}
public: 
  Token op;
  std::shared_ptr<Tacky> src1;
  std::shared_ptr<Tacky> src2;
  std::shared_ptr<Tacky> dest;
};

class TackyConstInt32 {
public: 
  TackyConstInt32(int value) :
    value(value) {}
public: 
  int value;
};

class TackyConstInt64 {
public: 
  TackyConstInt64(long value) :
    value(value) {}
public: 
  long value;
};

class TackyVar {
public: 
  TackyVar(std::string identifier) :
    identifier(identifier) {}
public: 
  std::string identifier;
};

class TackyReturn {
public: 
  TackyReturn(std::shared_ptr<Tacky> value) :
    value(value) {}
public: 
  std::shared_ptr<Tacky> value;
};

class TackyTruncate {
public: 
  TackyTruncate(std::shared_ptr<Tacky> src, std::shared_ptr<Tacky> dest) :
    src(src), dest(dest) {}
public: 
  std::shared_ptr<Tacky> src;
  std::shared_ptr<Tacky> dest;
};

class TackySignExtend {
public: 
  TackySignExtend(std::shared_ptr<Tacky> src, std::shared_ptr<Tacky> dest) :
    src(src), dest(dest) {}
public: 
  std::shared_ptr<Tacky> src;
  std::shared_ptr<Tacky> dest;
};

class TackyCopy {
public: 
  TackyCopy(std::shared_ptr<Tacky> src, std::shared_ptr<Tacky> dest) :
    src(src), dest(dest) {}
public: 
  std::shared_ptr<Tacky> src;
  std::shared_ptr<Tacky> dest;
};

class TackyJump {
public: 
  TackyJump(std::shared_ptr<Tacky> target) :
    target(target) {}
public: 
  std::shared_ptr<Tacky> target;
};

class TackyJumpIfZero {
public: 
  TackyJumpIfZero(std::shared_ptr<Tacky> condition, std::shared_ptr<Tacky> target) :
    condition(condition), target(target) {}
public: 
  std::shared_ptr<Tacky> condition;
  std::shared_ptr<Tacky> target;
};

class TackyJumpIfNotZero {
public: 
  TackyJumpIfNotZero(std::shared_ptr<Tacky> condition, std::shared_ptr<Tacky> target) :
    condition(condition), target(target) {}
public: 
  std::shared_ptr<Tacky> condition;
  std::shared_ptr<Tacky> target;
};

class TackyLabel {
public: 
  TackyLabel(std::string identifier) :
    identifier(identifier) {}
public: 
  std::string identifier;
};

class TackyFunCall {
public: 
  TackyFunCall(std::string fname, std::vector<std::shared_ptr<Tacky>> args, std::shared_ptr<Tacky> dest) :
    fname(fname), args(std::move(args)), dest(dest) {}
public: 
  std::string fname;
  std::vector<std::shared_ptr<Tacky>> args;
  std::shared_ptr<Tacky> dest;
};

} // end namespace

#endif
