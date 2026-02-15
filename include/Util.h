#ifndef UTIL_H
#define UTIL_H

#include "Token.h"
#include "ast/Asm.h"
#include "ast/Expr.h"
#include "ast/Stmt.h"
#include <format>
#include <memory>
#include <vector>
#include <algorithm>

namespace ccomp {
template<typename T>
bool one_of(const T& val, const std::vector<T>& vals) {
    return std::find(vals.begin(), vals.end(), val) != vals.end();
}

inline bool isRelationalOp(TokenType op) {
  return one_of(op, {TokenType::EQUAL_EQUAL, TokenType::BANG_EQUAL,
                TokenType::LESS, TokenType::LESS_EQUAL, TokenType::GREATER,
                TokenType::GREATER_EQUAL});
}

inline bool isLogicalOp(TokenType op) {
  return one_of(op, {TokenType::AMPERSAND_AMPERSAND,
                TokenType::PIPE_PIPE});
}

inline std::string toStr(const Function& fn) {
  return fn.name.toString();
}

inline std::string toStr(const Variable& var) {
  int level = var.level;
  return std::format("{}_scope_level{}", var.name.toString(), level);
}

inline std::string toStr(const FunctionParam& var) {
  int level = var.level;
  return std::format("{}_scope_level{}", var.name.toString(), level);
}

// From https://stackoverflow.com/a/3407254
inline int roundUp(int num, int multiple) {
  if (multiple == 0) {
    return num;
  }

  int remainder = abs(num) % multiple;
  if (remainder == 0) {
    return num;
  }

  if (num < 0) {
    return -(abs(num) - remainder);
  } else {
    return num + multiple - remainder;
  }
}

template<typename T, typename... Args>
std::shared_ptr<Asm> make_asm(Args&&... args)
{ return std::make_shared<Asm>(T(std::forward<Args>(args)...)); }

template<typename T, typename... Args>
void add_inst(std::vector<std::shared_ptr<Asm>>& instructions, Args&&... args)
{
  auto inst = std::make_shared<Asm>(T(std::forward<Args>(args)...));
  instructions.emplace_back(inst);
}

template<typename T, typename... Args>
std::shared_ptr<Asm> make_add_and_return(
  std::vector<std::shared_ptr<Asm>>& instructions, Args&&... args)
{
  auto inst = std::make_shared<Asm>(T(std::forward<Args>(args)...));
  instructions.emplace_back(inst);
  return inst;
}
}

#endif
