#include "Type.h"

using namespace ccomp;

const BuiltInType BuiltInType::int32Ty_(INT32);
const BuiltInType BuiltInType::int64Ty_(INT64);
const BuiltInType BuiltInType::boolTy_(BOOLEAN);

BuiltInType::BuiltInType(Kind kind) :
  kind_(kind)
{}

const Type* BuiltInType::getInt32Ty() {
  return &int32Ty_;
}

const Type* BuiltInType::getInt64Ty() {
  return &int64Ty_;
}

const Type* BuiltInType::getBoolTy() {
  return &boolTy_;
}

FunctionType::FunctionType(const Type* returnType, std::vector<const Type*> paramTypes) :
  returnType(returnType), paramTypes(std::move(paramTypes))
{}

const Type* FunctionType::getReturnTy() const {
  return returnType;
}

const std::vector<const Type*>& FunctionType::getParamTy() const {
  return paramTypes;
}
