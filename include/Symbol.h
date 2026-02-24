#ifndef SYMBOL_H
#define SYMBOL_H

#include <memory>

namespace ccomp {
class Function;
class FunctionParam;
class Variable;
class Scope;
class Type;

struct InitialValue {
  static const int TENTATIVE_VALUE = 0;
  static const int INITIAL_INT32_VALUE = 1;
  static const int INITIAL_LONG_VALUE = 2;
  static const int NOINIT_VALUE = 3;

  InitialValue() :
    type_(NOINIT_VALUE), value_(0) {}

  InitialValue(int type, long value=0) :
    type_(type), value_(value) {}

  int getType() const { return type_; }
  long getValue() const { return value_; }

private:
  int type_;
  long value_;
};

struct SymbolAttrs {
  static const int FUNCTION_ATTR = 0;
  static const int STATIC_ATTR = 1;
  static const int LOCAL_ATTR = 2;

  // Function attribute
  SymbolAttrs(bool defined, bool global) :
    attrType_(FUNCTION_ATTR), defined_(defined), global_(global)
  {}

  // Static attribute
  SymbolAttrs(InitialValue init, bool global) :
    attrType_(STATIC_ATTR), defined_(false), global_(global), init_(init)
  {}

  // Local attribute
  SymbolAttrs() :
    attrType_(LOCAL_ATTR), defined_(false), global_(false)
  {}

  int getAttributeType() const { return attrType_; }
  bool isDefined() const { return defined_; }
  bool isGlobal() const { return global_; }
  const InitialValue& getInitValue() const { return init_; }

private:
  const int attrType_;
  bool defined_;
  bool global_;
  InitialValue init_;
};

class Symbol {
public:
  Symbol(const std::string& name, bool hasExternalLinkage, std::shared_ptr<Scope> scope);
  virtual ~Symbol() = default;

  const std::string& getName() const;
  bool hasExternalLinkage() const;
  int getNestingLevel() const;
  std::shared_ptr<Scope> getScope() const;

  virtual bool isFunction() const;
  virtual const Function* getFunction() const;

  virtual bool isFunctionParam() const;
  virtual const FunctionParam* getFunctionParam() const;

  virtual bool isVariable() const;
  virtual const Variable* getVariable() const;

  void setType(const Type*);
  const Type* getType() const;

  void setAttrs(std::unique_ptr<SymbolAttrs> attrs);
  const SymbolAttrs* getAttrs() const;

private:
  std::string name_;
  bool hasExternalLinkage_;
  std::shared_ptr<Scope> scope_;
  const Type* ty_;
  std::unique_ptr<SymbolAttrs> attrs_;
};

struct FunctionSymbol : public Symbol {
  FunctionSymbol(const std::string& name, bool hasExternalLinkage, const Function* fn,
                 std::shared_ptr<Scope> scope);
  virtual ~FunctionSymbol() = default;

  bool isFunction() const override;
  const Function* getFunction() const override;

private:
  const Function* fn_;
};

struct FunctionParamSymbol : public Symbol {
  FunctionParamSymbol(const std::string& name, bool hasExternalLinkage,
                      const FunctionParam* param,
                      std::shared_ptr<Scope> scope);
  virtual ~FunctionParamSymbol() = default;

  bool isFunctionParam() const override;
  const FunctionParam* getFunctionParam() const override;

private:
  const FunctionParam* param_;
};

struct VariableSymbol : public Symbol {
  VariableSymbol(const std::string& name, bool hasExternalLinkage,
                 const Variable* var,
                 std::shared_ptr<Scope> scope);
  virtual ~VariableSymbol() = default;

  bool isVariable() const override;
  const Variable* getVariable() const override;

private:
  const Variable* var_;
};

struct StructSymbol : public Symbol {
  StructSymbol(const std::string& name) : Symbol(name, true, nullptr) {}
};
} // namespace ccomp

#endif // SYMBOL_H
