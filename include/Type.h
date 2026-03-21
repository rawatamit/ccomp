#ifndef TYPE_H
#define TYPE_H

#include <vector>

namespace ccomp {
class Type {
public:
  Type(int size, bool isSigned);
  virtual ~Type() = default;
  int getSize() const;
  bool isSigned() const;
private:
  // Size in bytes.
  int size_;
  bool isSigned_;
};

class BuiltInType : public Type {
private:
  enum Kind {
    INT32 = 0,
    UINT32,
    INT64,
    UINT64,
    BOOLEAN,
    TYPE_ERROR
  };

  BuiltInType(Kind kind, int size, bool isSigned);
  virtual ~BuiltInType() = default;

public:
  static const Type* getInt32Ty();
  static const Type* getUInt32Ty();
  static const Type* getInt64Ty();
  static const Type* getUInt64Ty();
  static const Type* getBoolTy();

private:
  Kind kind_;
  const static BuiltInType int32Ty_;
  const static BuiltInType uint32Ty_;
  const static BuiltInType int64Ty_;
  const static BuiltInType uint64Ty_;
  const static BuiltInType boolTy_;
};

class FunctionType : public Type {
public:
  FunctionType(const Type* returnType, std::vector<const Type*> paramTypes);
  virtual ~FunctionType() = default;

  const Type* getReturnTy() const;
  const std::vector<const Type*>& getParamTy() const;

private:
  const Type* returnType;
  std::vector<const Type*> paramTypes;
};
} // namespace ccomp

inline bool operator==(const ccomp::FunctionType& a, const ccomp::FunctionType& b) {
  if (a.getReturnTy() != b.getReturnTy()) {
    return false;
  }

  auto aParamTy = a.getParamTy();
  auto bParamTy = b.getParamTy();
  if (bParamTy.size() != aParamTy.size()) {
    return false;
  }

  for (size_t i = 0; i < aParamTy.size(); ++i) {
    if (aParamTy[i] != bParamTy[i]) {
      return false;
    }
  }

  return true;
}

#endif // TYPE_H
