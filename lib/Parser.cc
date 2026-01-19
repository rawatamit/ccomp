#include "Parser.h"
#include "ErrorHandler.h"
#include "Token.h"
#include <stdexcept>
#include <sys/cdefs.h>

using namespace ccomp;

ParseError::ParseError(std::string msg, Token token)
    : std::runtime_error(msg), token_(token) {}

Parser::Parser(const std::vector<Token> &tokens, ErrorHandler &errorHandler)
    : current(0), tokens_(tokens), errorHandler_(errorHandler) {}

std::shared_ptr<Stmt> Parser::declaration() {
  try {
#if 0
    if (match({TokenType::VAR})) {
      return varDeclaration();
    } else if (match({TokenType::FUN})) {
      return std::dynamic_pointer_cast<Stmt>(function());
    } else if (match({TokenType::CLASS})) {
      return classDecl();
    } else {
#endif
      return statement();
#if 0
    }
#endif
  } catch (const ParseError &e) {
    synchronize();
    return nullptr;
  }
}

std::shared_ptr<Function> Parser::function() {
  Token returnType = consume(TokenType::INT, "Expected return type.");
  Token name = consume(TokenType::IDENTIFIER, "Expected function name.");

  consume(TokenType::LEFT_PAREN, "expect '(' after function name.");

  std::vector<Token> params;
  if (!check(TokenType::RIGHT_PAREN)) {
    do {
      if (params.size() >= 255) {
        error(peek(), "can't have >= 255 parameters.");
      } else {
        params.push_back(
            //consume(TokenType::IDENTIFIER, "Expected parameter name."));
            consume(TokenType::VOID, "Expected parameter name."));
      }
    } while (match({TokenType::COMMA}));
  }

  consume(TokenType::RIGHT_PAREN, "Expected ')' after parameters.");

  consume(TokenType::LEFT_BRACE, "Expected '{' before function body.");
  std::shared_ptr<Block> body =
      std::static_pointer_cast<Block>(blockStatement());
  return std::make_shared<Function>(name, params, body->stmts);
}

std::shared_ptr<Stmt> Parser::varDeclaration() {
  Token name = consume(TokenType::IDENTIFIER, "Expected variable name.");

  std::shared_ptr<Expr> init = nullptr;
  if (match({TokenType::EQUAL})) {
    init = expression();
  }

  consume(TokenType::SEMICOLON, "expect ';' in var init.");
  return std::static_pointer_cast<Stmt>(std::make_shared<Var>(name, init));
}

std::shared_ptr<Stmt> Parser::statement() {
  switch (peek().type) {
#if 0
  case TokenType::PRINT:
    match({TokenType::PRINT});
    return printStatement();
  case TokenType::WHILE:
    match({TokenType::WHILE});
    return whileStatement();
  case TokenType::FOR:
    match({TokenType::FOR});
    return forStatement();
  case TokenType::LEFT_BRACE:
    match({TokenType::LEFT_BRACE});
    return blockStatement();
  case TokenType::IF:
    match({TokenType::IF});
    return ifStatement();
#endif
  case TokenType::RETURN:
    match({TokenType::RETURN});
    return returnStatement();
  default:
    return expressionStatement();
  }
}

std::shared_ptr<Stmt> Parser::ifStatement() {
  consume(TokenType::LEFT_PAREN, "need '(' in condition for if");
  auto condition = expression();
  consume(TokenType::RIGHT_PAREN, "need ')' in condition for if");

  std::shared_ptr<Stmt> thenBranch = statement();
  std::shared_ptr<Stmt> elseBranch = nullptr;

  if (match({TokenType::ELSE})) {
    elseBranch = statement();
  }

  return std::static_pointer_cast<Stmt>(
      std::make_shared<If>(condition, thenBranch, elseBranch));
}

std::shared_ptr<Stmt> Parser::printStatement() {
  auto val = expression();
  consume(TokenType::SEMICOLON, "Expected ';' after print.");
  return std::static_pointer_cast<Stmt>(std::make_shared<Print>(val));
}

std::shared_ptr<Stmt> Parser::whileStatement() {
  consume(TokenType::LEFT_PAREN, "need '(' in condition for while");
  auto condition = expression();
  consume(TokenType::RIGHT_PAREN, "need ')' in condition for while");

  auto body = statement();
  return std::static_pointer_cast<Stmt>(
      std::make_shared<While>(condition, body));
}

std::shared_ptr<Stmt> Parser::forStatement() {
  consume(TokenType::LEFT_PAREN, "Expected '(' after for");

  std::shared_ptr<Stmt> init = nullptr;
  switch (peek().type) {
  case TokenType::VAR:
    match({TokenType::VAR});
    init = varDeclaration();
    break;
  case TokenType::SEMICOLON:
    consume(TokenType::SEMICOLON, "Expected ';' in init");
    break;
  default:
    init = expressionStatement();
    break;
  }

  std::shared_ptr<Expr> condition = nullptr;
  if (!check(TokenType::SEMICOLON)) {
    condition = expression();
  }
  consume(TokenType::SEMICOLON, "Expected ';' after condition");

  std::shared_ptr<Expr> step = nullptr;
  if (!check(TokenType::RIGHT_PAREN)) {
    step = expression();
  }
  consume(TokenType::RIGHT_PAREN, "Expected ')' after update");

  auto body = statement();

  // init
  // while (condition)
  //   body
  //   step
  if (step != nullptr) {
    std::vector<std::shared_ptr<Stmt>> v;
    v.push_back(body);
    v.push_back(
        std::static_pointer_cast<Stmt>(std::make_shared<Expression>(step)));
    body = std::static_pointer_cast<Stmt>(std::make_shared<Block>(v));
  }

  if (condition == nullptr) {
    condition = std::static_pointer_cast<Expr>(
        std::make_shared<LiteralExpr>(TokenType::TRUE, "true"));
  }

  body =
      std::static_pointer_cast<Stmt>(std::make_shared<While>(condition, body));

  if (init != nullptr) {
    std::vector<std::shared_ptr<Stmt>> v;
    v.push_back(init);
    v.push_back(body);
    body = std::static_pointer_cast<Stmt>(std::make_shared<Block>(v));
  }

  return body;
}

std::shared_ptr<Stmt> Parser::blockStatement() {
  std::vector<std::shared_ptr<Stmt>> stmts;

  while (!check(TokenType::RIGHT_BRACE) and !isAtEnd()) {
    stmts.push_back(declaration());
  }

  consume(TokenType::RIGHT_BRACE, "Expected '}' after block");
  return std::static_pointer_cast<Stmt>(std::make_shared<Block>(stmts));
}

std::shared_ptr<Stmt> Parser::expressionStatement() {
  auto val = expression();
  consume(TokenType::SEMICOLON, "Expected ';' after expression.");
  return std::static_pointer_cast<Stmt>(std::make_shared<Expression>(val));
}

std::shared_ptr<Stmt> Parser::returnStatement() {
  Token keyword = previous();
  std::shared_ptr<Expr> expr = nullptr;
  if (!check(TokenType::SEMICOLON)) {
    expr = expression();
  }
  consume(TokenType::SEMICOLON, "Expected ';' after return.");
  return std::static_pointer_cast<Stmt>(
      std::make_shared<Return>(keyword, expr));
}

std::shared_ptr<Expr> Parser::expression() { return assignment(); }

std::shared_ptr<Expr> Parser::assignment() {
  auto expr = logic_or();

  if (match({TokenType::EQUAL})) {
    Token equals = previous();
    auto value = assignment();

    if (auto var = std::dynamic_pointer_cast<Variable>(expr)) {
      Token name = var->name;
      return std::static_pointer_cast<Expr>(
          std::make_shared<Assign>(name, value));
    }

    error(equals, "Invalid assignment target.");
  }

  return expr;
}

std::shared_ptr<Expr> Parser::logic_or() {
  auto expr = logic_and();

  while (match({TokenType::OR})) {
    Token op = previous();
    auto rhs = logic_and();
    expr = std::static_pointer_cast<Expr>(
        std::make_shared<Logical>(expr, op, rhs));
  }

  return expr;
}

std::shared_ptr<Expr> Parser::logic_and() {
  auto expr = equality();

  while (match({TokenType::AND})) {
    Token op = previous();
    auto rhs = equality();
    expr = std::static_pointer_cast<Expr>(
        std::make_shared<Logical>(expr, op, rhs));
  }

  return expr;
}

std::shared_ptr<Expr> Parser::equality() {
  auto expr = comparison();
  while (match({TokenType::BANG_EQUAL, TokenType::EQUAL_EQUAL})) {
    Token Operator = previous();
    auto right = comparison();
    expr = std::static_pointer_cast<Expr>(
        std::make_shared<BinaryExpr>(expr, Operator, right));
  }
  return expr;
}

std::shared_ptr<Expr> Parser::comparison() {
  auto expr = term();
  while (match({TokenType::GREATER, TokenType::LESS, TokenType::LESS_EQUAL,
                TokenType::GREATER_EQUAL})) {
    Token Operator = previous();
    auto right = term();
    expr = std::static_pointer_cast<Expr>(
        std::make_shared<BinaryExpr>(expr, Operator, right));
  }
  return expr;
}

std::shared_ptr<Expr> Parser::term() {
  auto expr = factor();
  while (match({TokenType::MINUS, TokenType::PLUS})) {
    Token Operator = previous();
    auto right = factor();
    expr = std::static_pointer_cast<Expr>(
        std::make_shared<BinaryExpr>(expr, Operator, right));
  }
  return expr;
}

std::shared_ptr<Expr> Parser::factor() {
  auto expr = unary();
  while (match({TokenType::SLASH, TokenType::STAR, TokenType::PERCENT})) {
    Token Operator = previous();
    auto right = unary();
    expr = std::static_pointer_cast<Expr>(
        std::make_shared<BinaryExpr>(expr, Operator, right));
  }
  return expr;
}

std::shared_ptr<Expr> Parser::unary() {
  if (match({TokenType::TILDE, TokenType::BANG, TokenType::MINUS})) {
    Token Operator = previous();
    auto right = unary();
    return std::static_pointer_cast<Expr>(
        std::make_shared<UnaryExpr>(Operator, right));
  }
  return call();
}

std::shared_ptr<Expr> Parser::call() {
  auto e = primary();

  while (true) {
    if (match({TokenType::LEFT_PAREN})) {
      // function call must begin with an identifier
      if (auto le = std::dynamic_pointer_cast<LiteralExpr>(e)) {
        throw error(previous(), "Expected identifier in call expression.");
      }
      e = finishCall(e);
    } else if (match({TokenType::DOT})) {
      Token name =
          consume(TokenType::IDENTIFIER, "Expected property name after '.'.");
      // e = std::static_pointer_cast<Expr>(std::make_shared<Get>(e, name));
    } else {
      break;
    }
  }

  return e;
}

std::shared_ptr<Expr> Parser::finishCall(std::shared_ptr<Expr> e __attribute_maybe_unused__) {
  std::vector<std::shared_ptr<Expr>> args;
  if (!check(TokenType::RIGHT_PAREN)) {
    do {
      if (args.size() >= 255) {
        error(peek(), "cannot have more than 255 arguments");
      } else {
        args.push_back(expression());
      }
    } while (match({TokenType::COMMA}));
  }

  Token paren = consume(TokenType::RIGHT_PAREN, "expected ')' in call");
  return nullptr;
  // return std::static_pointer_cast<Expr>(std::make_shared<Call>(e, paren, args));
}

std::shared_ptr<Expr> Parser::primary() {
  if (match({TokenType::FALSE}))
    return std::make_shared<LiteralExpr>(TokenType::FALSE, "false");
  if (match({TokenType::TRUE}))
    return std::make_shared<LiteralExpr>(TokenType::TRUE, "true");
  if (match({TokenType::NIL}))
    return std::make_shared<LiteralExpr>(TokenType::NIL, "nil");
  if (match({TokenType::NUMBER, TokenType::STRING}))
    return std::make_shared<LiteralExpr>(previous().type, previous().literal);
  if (match({TokenType::LEFT_PAREN})) {
    auto expr = expression();
    consume(TokenType::RIGHT_PAREN, "Expected ')' after expression.");
    return expr;
    // return std::static_pointer_cast<Expr>(std::make_shared<GroupingExpr>(expr));
  }
  if (match({TokenType::IDENTIFIER})) {
    return std::make_shared<Variable>(previous());
  }
  throw error(peek(), "Expected expression.");
  return nullptr;
}

std::vector<std::shared_ptr<Stmt>> Parser::parse() {
  std::vector<std::shared_ptr<Stmt>> stmts;

  while (!isAtEnd()) {
    //stmts.push_back(declaration());
    stmts.push_back(function());
  }

  return stmts;
}

Token Parser::consume(TokenType type, const std::string &message) {
  if (check(type))
    return advance();
  throw error(peek(), message);
}

ParseError Parser::error(Token token, std::string message) {
  if (token.type == TokenType::END_OF_FILE) {
    errorHandler_.add(token.line, " at end", message);
  } else {
    errorHandler_.add(token.line, " at '" + token.lexeme + "'", message);
  }
  return ParseError(message, token);
}

bool Parser::match(const std::vector<TokenType> &types) {
  for (auto type : types) {
    if (check(type)) {
      advance();
      return true;
    }
  }
  return false;
}

Token Parser::previous() { return tokens_[current - 1]; }

Token Parser::advance() {
  if (!isAtEnd())
    ++current;
  return previous();
}

Token Parser::peek() { return tokens_[current]; }

bool Parser::isAtEnd() { return peek().type == TokenType::END_OF_FILE; }

bool Parser::check(TokenType type) {
  if (isAtEnd())
    return false;
  return peek().type == type;
}

void Parser::synchronize() {
  advance();

  while (!isAtEnd()) {
    if (previous().type == TokenType::SEMICOLON)
      return;

    switch (peek().type) {
    case TokenType::CLASS:
    case TokenType::FUN:
    case TokenType::VAR:
    case TokenType::FOR:
    case TokenType::IF:
    case TokenType::WHILE:
    case TokenType::PRINT:
    case TokenType::RETURN:
      return;
    default:
      break;
    }

    advance();
  }
}
