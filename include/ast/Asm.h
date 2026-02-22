#ifndef Asm_H_
#define Asm_H_

#include "Token.h"
#include <memory>
#include <vector>
#include <string>
#include <variant>

namespace ccomp {
class AsmProgram;
class AsmFunction;
class AsmStaticVar;
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
class AsmDeallocateStack;
class AsmPush;
class AsmCall;
class AsmReturn;
class AsmImm;
class AsmRegister;
class AsmPseudo;
class AsmStack;
class AsmData;
using Asm = std::variant<AsmProgram, AsmFunction, AsmStaticVar, AsmUnary, AsmBinary, AsmCmp, AsmIdiv, AsmCdq, AsmJmp, AsmJmpCC, AsmSetCC, AsmLabel, AsmMov, AsmAllocateStack, AsmDeallocateStack, AsmPush, AsmCall, AsmReturn, AsmImm, AsmRegister, AsmPseudo, AsmStack, AsmData>;
enum AsmCondCode {
  E,
  NE,
  G,
  GE,
  L,
  LE,
};
enum AsmReg {
  AX,
  CX,
  DX,
  DI,
  SI,
  R8,
  R9,
  R10,
  R11,
};
enum AsmWordSize {
  QUAD,
  LONG,
  BYTE,
};
class AsmProgram {
public: 
  AsmProgram(std::vector<std::shared_ptr<Asm>> functions) :
    functions(std::move(functions)) {}
public: 
  std::vector<std::shared_ptr<Asm>> functions;
};

class AsmFunction {
public: 
  AsmFunction(bool global, std::string name, std::vector<std::shared_ptr<Asm>> instructions) :
    global(global), name(name), instructions(std::move(instructions)) {}
public: 
  bool global;
  std::string name;
  std::vector<std::shared_ptr<Asm>> instructions;
};

class AsmStaticVar {
public: 
  AsmStaticVar(bool global, std::string name, int init) :
    global(global), name(name), init(init) {}
public: 
  bool global;
  std::string name;
  int init;
};

class AsmUnary {
public: 
  AsmUnary(Token op, std::shared_ptr<Asm> operand) :
    op(op), operand(operand) {}
public: 
  Token op;
  std::shared_ptr<Asm> operand;
};

class AsmBinary {
public: 
  AsmBinary(Token op, std::shared_ptr<Asm> operand1, std::shared_ptr<Asm> operand2) :
    op(op), operand1(operand1), operand2(operand2) {}
public: 
  Token op;
  std::shared_ptr<Asm> operand1;
  std::shared_ptr<Asm> operand2;
};

class AsmCmp {
public: 
  AsmCmp(std::shared_ptr<Asm> operand1, std::shared_ptr<Asm> operand2) :
    operand1(operand1), operand2(operand2) {}
public: 
  std::shared_ptr<Asm> operand1;
  std::shared_ptr<Asm> operand2;
};

class AsmIdiv {
public: 
  AsmIdiv(std::shared_ptr<Asm> operand) :
    operand(operand) {}
public: 
  std::shared_ptr<Asm> operand;
};

class AsmCdq {
public: 
  AsmCdq(int dummy) :
    dummy(dummy) {}
public: 
  int dummy;
};

class AsmJmp {
public: 
  AsmJmp(std::shared_ptr<Asm> target) :
    target(target) {}
public: 
  std::shared_ptr<Asm> target;
};

class AsmJmpCC {
public: 
  AsmJmpCC(AsmCondCode cond_code, std::shared_ptr<Asm> target) :
    cond_code(cond_code), target(target) {}
public: 
  AsmCondCode cond_code;
  std::shared_ptr<Asm> target;
};

class AsmSetCC {
public: 
  AsmSetCC(AsmCondCode cond_code, std::shared_ptr<Asm> operand) :
    cond_code(cond_code), operand(operand) {}
public: 
  AsmCondCode cond_code;
  std::shared_ptr<Asm> operand;
};

class AsmLabel {
public: 
  AsmLabel(std::string identifier) :
    identifier(identifier) {}
public: 
  std::string identifier;
};

class AsmMov {
public: 
  AsmMov(std::shared_ptr<Asm> src, std::shared_ptr<Asm> dest) :
    src(src), dest(dest) {}
public: 
  std::shared_ptr<Asm> src;
  std::shared_ptr<Asm> dest;
};

class AsmAllocateStack {
public: 
  AsmAllocateStack(int size) :
    size(size) {}
public: 
  int size;
};

class AsmDeallocateStack {
public: 
  AsmDeallocateStack(int size) :
    size(size) {}
public: 
  int size;
};

class AsmPush {
public: 
  AsmPush(std::shared_ptr<Asm> operand) :
    operand(operand) {}
public: 
  std::shared_ptr<Asm> operand;
};

class AsmCall {
public: 
  AsmCall(std::string fname) :
    fname(fname) {}
public: 
  std::string fname;
};

class AsmReturn {
public: 
  AsmReturn(int dummy) :
    dummy(dummy) {}
public: 
  int dummy;
};

class AsmImm {
public: 
  AsmImm(int value) :
    value(value) {}
public: 
  int value;
};

class AsmRegister {
public: 
  AsmRegister(AsmReg reg, AsmWordSize size) :
    reg(reg), size(size) {}
public: 
  AsmReg reg;
  AsmWordSize size;
};

class AsmPseudo {
public: 
  AsmPseudo(std::string identifier) :
    identifier(identifier) {}
public: 
  std::string identifier;
};

class AsmStack {
public: 
  AsmStack(int offset) :
    offset(offset) {}
public: 
  int offset;
};

class AsmData {
public: 
  AsmData(std::string identifier) :
    identifier(identifier) {}
public: 
  std::string identifier;
};

} // end namespace

#endif
