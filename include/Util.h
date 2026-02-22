#ifndef UTIL_H
#define UTIL_H

#include "Token.h"
#include "ast/Asm.h"
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

inline bool isStorageQualifier(const Token& tok) {
  return (tok.type == TokenType::STATIC || tok.type == TokenType::EXTERN);
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
std::shared_ptr<Asm> make_asm(Args&&... args) {
  return std::make_shared<Asm>(T(std::forward<Args>(args)...));
}

template<typename T, typename... Args>
void add_inst(std::vector<std::shared_ptr<Asm>>& instructions, Args&&... args) {
  auto inst = std::make_shared<Asm>(T(std::forward<Args>(args)...));
  instructions.emplace_back(inst);
}

template<typename T, typename... Args>
std::shared_ptr<Asm> make_add_and_return(
  std::vector<std::shared_ptr<Asm>>& instructions, Args&&... args) {
  auto inst = std::make_shared<Asm>(T(std::forward<Args>(args)...));
  instructions.emplace_back(inst);
  return inst;
}

inline bool isMemoryValue(std::shared_ptr<Asm> inst) {
  return (std::holds_alternative<AsmData>(*inst) ||
          std::holds_alternative<AsmStack>(*inst));
}
}

#endif
