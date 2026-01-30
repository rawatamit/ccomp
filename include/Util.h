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
