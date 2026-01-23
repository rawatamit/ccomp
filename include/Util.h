#ifndef UTIL_H
#define UTIL_H

#include "Token.h"
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
}

#endif