#ifndef _COBJECT_H_
#define _COBJECT_H_

#include <memory>
#include <string>

namespace ccomp {

class CObject {
public:
  enum ObjT {
    DOUBLE,
    STRING,
    NIL,
    BOOL,
    CLASS,
    FUNCTION,
    INSTANCE,
    RETURN,
    BUILTIN,
  };

private:
  ObjT type;

public:
  CObject(ObjT type) : type(type) {}

  virtual ~CObject() = default;

  ObjT getType() const { return type; }

  virtual bool isTruthy() const { return true; }

  virtual std::string str() const { return "<CObject>"; }

  virtual bool isEqual(std::shared_ptr<CObject>) const { return false; }
};

} // namespace ccomp

#endif
