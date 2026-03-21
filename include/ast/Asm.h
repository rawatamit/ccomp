#ifndef Asm_H_
#define Asm_H_

#include "Token.h"
#include "Symbol.h"
#include <memory>
#include <vector>
#include <string>
#include <variant>

namespace ccomp {
class Type;
class AsmProgram;
class AsmFunction;
class AsmStaticVar;
class AsmUnary;
class AsmBinary;
class AsmCmp;
class AsmIdiv;
class AsmDiv;
class AsmCdq;
class AsmJmp;
class AsmJmpCC;
class AsmSetCC;
class AsmLabel;
class AsmMov;
class AsmMovsx;
class AsmMovZeroExtend;
class AsmPush;
class AsmCall;
class AsmReturn;
class AsmImm;
class AsmRegister;
class AsmPseudo;
class AsmStack;
class AsmData;
typedef const Type* type_ptr;
using Asm = std::variant<AsmProgram, AsmFunction, AsmStaticVar, AsmUnary, AsmBinary, AsmCmp, AsmIdiv, AsmDiv, AsmCdq, AsmJmp, AsmJmpCC, AsmSetCC, AsmLabel, AsmMov, AsmMovsx, AsmMovZeroExtend, AsmPush, AsmCall, AsmReturn, AsmImm, AsmRegister, AsmPseudo, AsmStack, AsmData>;
enum AsmCondCode {
  E,
  NE,
  G,
  GE,
  L,
  LE,
  A,
  AE,
  B,
  BE,
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
  SP,
};
enum AsmInst {
  SUB,
  ADD,
  MUL,
  NOT,
  NEG,
  MOV,
  PUSH,
  CALL,
  CDQ,
  IDIV,
  DIV,
  JMP,
  CMP,
  INST_ERR,
};
enum AsmInstType {
  QUAD,
  LONG,
  BYTE,
  ASM_TYPE_ERR,
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
  AsmStaticVar(bool global, std::string name, int alignment, InitialValue init) :
    global(global), name(name), alignment(alignment), init(init) {}
public: 
  bool global;
  std::string name;
  int alignment;
  InitialValue init;
};

class AsmUnary {
public: 
  AsmUnary(AsmInstType type, AsmInst op, std::shared_ptr<Asm> operand) :
    type(type), op(op), operand(operand) {}
public: 
  AsmInstType type;
  AsmInst op;
  std::shared_ptr<Asm> operand;
};

class AsmBinary {
public: 
  AsmBinary(AsmInstType type, AsmInst op, std::shared_ptr<Asm> operand1, std::shared_ptr<Asm> operand2) :
    type(type), op(op), operand1(operand1), operand2(operand2) {}
public: 
  AsmInstType type;
  AsmInst op;
  std::shared_ptr<Asm> operand1;
  std::shared_ptr<Asm> operand2;
};

class AsmCmp {
public: 
  AsmCmp(AsmInstType type, std::shared_ptr<Asm> operand1, std::shared_ptr<Asm> operand2) :
    type(type), operand1(operand1), operand2(operand2) {}
public: 
  AsmInstType type;
  std::shared_ptr<Asm> operand1;
  std::shared_ptr<Asm> operand2;
};

class AsmIdiv {
public: 
  AsmIdiv(AsmInstType type, std::shared_ptr<Asm> operand) :
    type(type), operand(operand) {}
public: 
  AsmInstType type;
  std::shared_ptr<Asm> operand;
};

class AsmDiv {
public: 
  AsmDiv(AsmInstType type, std::shared_ptr<Asm> operand) :
    type(type), operand(operand) {}
public: 
  AsmInstType type;
  std::shared_ptr<Asm> operand;
};

class AsmCdq {
public: 
  AsmCdq(AsmInstType type, int dummy) :
    type(type), dummy(dummy) {}
public: 
  AsmInstType type;
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
  AsmMov(AsmInstType type, std::shared_ptr<Asm> src, std::shared_ptr<Asm> dest) :
    type(type), src(src), dest(dest) {}
public: 
  AsmInstType type;
  std::shared_ptr<Asm> src;
  std::shared_ptr<Asm> dest;
};

class AsmMovsx {
public: 
  AsmMovsx(std::shared_ptr<Asm> src, std::shared_ptr<Asm> dest) :
    src(src), dest(dest) {}
public: 
  std::shared_ptr<Asm> src;
  std::shared_ptr<Asm> dest;
};

class AsmMovZeroExtend {
public: 
  AsmMovZeroExtend(std::shared_ptr<Asm> src, std::shared_ptr<Asm> dest) :
    src(src), dest(dest) {}
public: 
  std::shared_ptr<Asm> src;
  std::shared_ptr<Asm> dest;
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
  AsmImm(uint64_t value) :
    value(value) {}
public: 
  uint64_t value;
};

class AsmRegister {
public: 
  AsmRegister(AsmInstType type, AsmReg reg) :
    type(type), reg(reg) {}
public: 
  AsmInstType type;
  AsmReg reg;
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
