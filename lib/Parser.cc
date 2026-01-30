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

std::unique_ptr<Stmt> Parser::declaration() {
  try {
#if 1
    if (match({TokenType::INT})) {
      return varDeclaration();
    }
#endif
#if 0
    else if (match({TokenType::FUN})) {
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

Function Parser::function() {
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
  auto body = blockStatement();
  return Function(name, params, std::move(body.stmts));
}

std::unique_ptr<Stmt> Parser::varDeclaration() {
  // declarations must start with type
  match({TokenType::INT});
  Token name = consume(TokenType::IDENTIFIER, "Expected variable name.");

  std::unique_ptr<Expr> init;
  if (match({TokenType::EQUAL})) {
    init = expression();
  }

  consume(TokenType::SEMICOLON, "expect ';' in var init.");
  return std::make_unique<Stmt>(Decl(name, std::move(init)));
}

std::unique_ptr<Stmt> Parser::statement() {
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
#endif
  case TokenType::LEFT_BRACE:
    match({TokenType::LEFT_BRACE});
    return std::make_unique<Stmt>(blockStatement());
  case TokenType::IF:
    match({TokenType::IF});
    return ifStatement();
  case TokenType::SEMICOLON:
    match({TokenType::SEMICOLON});
    return std::make_unique<Stmt>(Null(previous()));
  case TokenType::RETURN:
    match({TokenType::RETURN});
    return returnStatement();
  default:
    return expressionStatement();
  }
}

std::unique_ptr<Stmt> Parser::ifStatement() {
  consume(TokenType::LEFT_PAREN, "need '(' in condition for if");
  auto condition = expression();
  consume(TokenType::RIGHT_PAREN, "need ')' in condition for if");

  std::unique_ptr<Stmt> thenBranch = statement();
  std::unique_ptr<Stmt> elseBranch;

  if (match({TokenType::ELSE})) {
    elseBranch = statement();
  }

  //return std::make_unique<Stmt>(If(std::move(condition), std::move(thenBranch), std::move(elseBranch)));
  return std::make_unique<Stmt>(If(std::move(condition), std::move(thenBranch), std::move(elseBranch)));
}

std::unique_ptr<Stmt> Parser::whileStatement() {
  consume(TokenType::LEFT_PAREN, "need '(' in condition for while");
  auto condition = expression();
  consume(TokenType::RIGHT_PAREN, "need ')' in condition for while");

  auto body = statement();
  return std::make_unique<Stmt>(While(std::move(condition), std::move(body)));
}

std::unique_ptr<Stmt> Parser::forStatement() {
  consume(TokenType::LEFT_PAREN, "Expected '(' after for");

  std::unique_ptr<Stmt> init = nullptr;
  switch (peek().type) {
  case TokenType::INT:
    match({TokenType::INT});
    init = varDeclaration();
    break;
  case TokenType::SEMICOLON:
    consume(TokenType::SEMICOLON, "Expected ';' in init");
    break;
  default:
    init = expressionStatement();
    break;
  }

  std::unique_ptr<Expr> condition = nullptr;
  if (!check(TokenType::SEMICOLON)) {
    condition = expression();
  }
  consume(TokenType::SEMICOLON, "Expected ';' after condition");

  std::unique_ptr<Expr> step = nullptr;
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
    std::vector<std::unique_ptr<Stmt>> v;
    v.push_back(std::move(body));
    v.push_back(std::make_unique<Stmt>(Expression(std::move(step))));
    body = std::make_unique<Stmt>(Block(std::move(v)));
  }

  if (condition == nullptr) {
    condition = std::make_unique<Expr>(LiteralExpr(TokenType::TRUE, "true"));
  }

  body = std::make_unique<Stmt>(While(std::move(condition), std::move(body)));

  if (init != nullptr) {
    std::vector<std::unique_ptr<Stmt>> v;
    v.push_back(std::move(init));
    v.push_back(std::move(body));
    body = std::make_unique<Stmt>(Block(std::move(v)));
  }

  return body;
}

Block Parser::blockStatement() {
  std::vector<std::unique_ptr<Stmt>> stmts;

  while (!check(TokenType::RIGHT_BRACE) and !isAtEnd()) {
    stmts.push_back(declaration());
  }

  consume(TokenType::RIGHT_BRACE, "Expected '}' after block");
  return Block(std::move(stmts));
}

std::unique_ptr<Stmt> Parser::expressionStatement() {
  auto val = expression();
  consume(TokenType::SEMICOLON, "Expected ';' after expression.");
  return std::make_unique<Stmt>(Expression(std::move(val)));
}

std::unique_ptr<Stmt> Parser::returnStatement() {
  Token keyword = previous();
  std::unique_ptr<Expr> expr = nullptr;
  if (!check(TokenType::SEMICOLON)) {
    expr = expression();
  }
  consume(TokenType::SEMICOLON, "Expected ';' after return.");
  return std::make_unique<Stmt>(Return(keyword, std::move(expr)));
}

std::unique_ptr<Expr> Parser::expression() { return assignment(); }

std::unique_ptr<Expr> Parser::assignment() {
  auto expr = conditional_ternary();

  while (match({TokenType::EQUAL})) {
    Token equals = previous();
    auto value = assignment();
    expr = std::make_unique<Expr>(Assign(std::move(expr), std::move(value)));
  }

  return expr;
}

std::unique_ptr<Expr> Parser::conditional_ternary() {
  auto expr = logic_or();

  while (match({TokenType::QUESTION_MARK})) {
    Token question_mark = previous();
    auto thenExp = expression();
    if (match({TokenType::COLON})) {
      auto elseExp = conditional_ternary();
      expr = std::make_unique<Expr>(Conditional(std::move(expr), std::move(thenExp), std::move(elseExp)));
    } else {
      error(question_mark, "Expected : after ? in conditional ternary.");
    }
  }

  return expr;
}

std::unique_ptr<Expr> Parser::logic_or() {
  auto expr = logic_and();

  while (match({TokenType::PIPE_PIPE})) {
    Token op = previous();
    auto rhs = logic_and();
    expr = std::make_unique<Expr>(BinaryExpr(std::move(expr), std::move(op), std::move(rhs)));
  }

  return expr;
}

std::unique_ptr<Expr> Parser::logic_and() {
  auto expr = equality();

  while (match({TokenType::AMPERSAND_AMPERSAND})) {
    Token op = previous();
    auto rhs = equality();
    expr = std::make_unique<Expr>(BinaryExpr(std::move(expr), op, std::move(rhs)));
  }

  return expr;
}

std::unique_ptr<Expr> Parser::equality() {
  auto expr = comparison();
  while (match({TokenType::BANG_EQUAL, TokenType::EQUAL_EQUAL})) {
    Token Operator = previous();
    auto right = comparison();
    expr = std::make_unique<Expr>(BinaryExpr(std::move(expr), Operator, std::move(right)));
  }
  return expr;
}

std::unique_ptr<Expr> Parser::comparison() {
  auto expr = term();
  while (match({TokenType::GREATER, TokenType::LESS, TokenType::LESS_EQUAL,
                TokenType::GREATER_EQUAL})) {
    Token Operator = previous();
    auto right = term();
    expr = std::make_unique<Expr>(BinaryExpr(std::move(expr), Operator, std::move(right)));
  }
  return expr;
}

std::unique_ptr<Expr> Parser::term() {
  auto expr = factor();
  while (match({TokenType::MINUS, TokenType::PLUS})) {
    Token Operator = previous();
    auto right = factor();
    expr = std::make_unique<Expr>(BinaryExpr(std::move(expr), Operator, std::move(right)));
  }
  return expr;
}

std::unique_ptr<Expr> Parser::factor() {
  auto expr = unary();
  while (match({TokenType::SLASH, TokenType::STAR, TokenType::PERCENT})) {
    Token Operator = previous();
    auto right = unary();
    expr = std::make_unique<Expr>(BinaryExpr(std::move(expr), Operator, std::move(right)));
  }
  return expr;
}

std::unique_ptr<Expr> Parser::unary() {
  if (match({TokenType::TILDE, TokenType::BANG, TokenType::MINUS})) {
    Token Operator = previous();
    auto right = unary();
    return std::make_unique<Expr>(UnaryExpr(Operator, std::move(right)));
  }
  return call();
}

std::unique_ptr<Expr> Parser::call() {
  auto e = primary();

  while (true) {
    if (match({TokenType::LEFT_PAREN})) {
      // function call must begin with an identifier
      if (!std::holds_alternative<Variable>(*e)) {
        throw error(previous(), "Expected identifier in call expression.");
      }
      e = finishCall(std::move(e));
    } else if (match({TokenType::DOT})) {
      Token name =
          consume(TokenType::IDENTIFIER, "Expected property name after '.'.");
      // e = std::static_pointer_cast<Expr>(std::make_unique<Get>(e, name));
    } else {
      break;
    }
  }

  return e;
}

std::unique_ptr<Expr> Parser::finishCall(std::unique_ptr<Expr>) {
  std::vector<std::unique_ptr<Expr>> args;
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
  // return std::static_pointer_cast<Expr>(std::make_unique<Call>(e, paren, args));
}

std::unique_ptr<Expr> Parser::primary() {
  if (match({TokenType::FALSE}))
    return std::make_unique<Expr>(LiteralExpr(TokenType::FALSE, "false"));
  if (match({TokenType::TRUE}))
    return std::make_unique<Expr>(LiteralExpr(TokenType::TRUE, "true"));
  if (match({TokenType::NUMBER, TokenType::STRING}))
    return std::make_unique<Expr>(LiteralExpr(previous().type, previous().literal));
  if (match({TokenType::LEFT_PAREN})) {
    auto expr = expression();
    consume(TokenType::RIGHT_PAREN, "Expected ')' after expression.");
    return expr;
    // return std::static_pointer_cast<Expr>(std::make_unique<GroupingExpr>(expr));
  }
  if (match({TokenType::IDENTIFIER})) {
    return std::make_unique<Expr>(Variable(previous()));
  }
  throw error(peek(), "Expected expression.");
  return nullptr;
}

std::vector<std::unique_ptr<Stmt>> Parser::parse() {
  std::vector<std::unique_ptr<Stmt>> stmts;

  while (!isAtEnd()) {
    //stmts.push_back(declaration());
    stmts.push_back(std::make_unique<Stmt>(function()));
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
