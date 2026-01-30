#ifndef PARSER_HPP
#define PARSER_HPP

#include "ast/Expr.h"
#include "ast/Stmt.h"
#include "Token.h"
#include <memory>
#include <stdexcept>
#include <vector>

namespace ccomp {
// forward declarations
class ErrorHandler;

class ParseError : public std::runtime_error {
public:
  ParseError(std::string msg, Token token);
  Token token_;
};

class Parser {
public:
  Parser(const std::vector<Token> &tokens, ErrorHandler &errorHandler);
  size_t current;
  std::unique_ptr<Stmt> declaration();
  Function function();
  Block blockStatement();
  std::unique_ptr<Stmt> varDeclaration();
  std::unique_ptr<Stmt> statement();
  std::unique_ptr<Stmt> ifStatement();
  std::unique_ptr<Stmt> whileStatement();
  std::unique_ptr<Stmt> doWhileStatement();
  std::unique_ptr<Stmt> forStatement();
  std::unique_ptr<Stmt> expressionStatement();
  std::unique_ptr<Stmt> returnStatement();
  std::unique_ptr<Expr> expression();
  std::unique_ptr<Expr> assignment();
  std::unique_ptr<Expr> conditional_ternary();
  std::unique_ptr<Expr> logic_or();
  std::unique_ptr<Expr> logic_and();
  std::unique_ptr<Expr> equality();
  std::unique_ptr<Expr> comparison();
  std::unique_ptr<Expr> term();
  std::unique_ptr<Expr> factor();
  std::unique_ptr<Expr> call();
  std::unique_ptr<Expr> finishCall(std::unique_ptr<Expr> e);
  std::unique_ptr<Expr> unary();
  std::unique_ptr<Expr> primary();
  std::vector<std::unique_ptr<Stmt>> parse();
  ParseError error(Token token, std::string message);

private:
  bool match(const std::vector<TokenType> &types);
  Token previous();
  Token advance();
  Token peek();
  bool isAtEnd();
  bool check(TokenType type);
  Token consume(TokenType type, const std::string &message);
  void synchronize();
  std::vector<Token> tokens_;
  ErrorHandler &errorHandler_;
};
} // namespace ccomp

#endif // PARSER_HPP
