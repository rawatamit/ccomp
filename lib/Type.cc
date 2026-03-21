#include "Type.h"

using namespace ccomp;

const BuiltInType BuiltInType::int32Ty_(INT32, 4, true);
const BuiltInType BuiltInType::uint32Ty_(UINT32, 4, false);
const BuiltInType BuiltInType::int64Ty_(INT64, 8, true);
const BuiltInType BuiltInType::uint64Ty_(UINT64, 8, false);
const BuiltInType BuiltInType::boolTy_(BOOLEAN, 1, false);

Type::Type(int size, bool isSigned) :
  size_(size), isSigned_(isSigned) {}

int Type::getSize() const {
  return size_;
}

bool Type::isSigned() const {
  return isSigned_;
}

BuiltInType::BuiltInType(Kind kind, int size, bool isSigned) :
  Type(size, isSigned), kind_(kind)
{}

const Type* BuiltInType::getInt32Ty() {
  return &int32Ty_;
}

const Type* BuiltInType::getUInt32Ty() {
  return &uint32Ty_;
}

const Type* BuiltInType::getInt64Ty() {
  return &int64Ty_;
}

const Type* BuiltInType::getUInt64Ty() {
  return &uint64Ty_;
}

const Type* BuiltInType::getBoolTy() {
  return &boolTy_;
}

FunctionType::FunctionType(const Type* returnType, std::vector<const Type*> paramTypes) :
  Type(0, false), returnType(returnType), paramTypes(std::move(paramTypes))
{}

const Type* FunctionType::getReturnTy() const {
  return returnType;
}

const std::vector<const Type*>& FunctionType::getParamTy() const {
  return paramTypes;
}
