#include "Parser.h"
#include "Util.h"
#include "ErrorHandler.h"
#include "Token.h"
#include <cassert>
#include <algorithm>
#include <stdexcept>

using namespace ccomp;

ParseError::ParseError(std::string msg, Token token)
    : std::runtime_error(msg), token_(token) {}

Parser::Parser(const std::vector<Token> &tokens, ErrorHandler &errorHandler)
    : current(0), tokens_(tokens), errorHandler_(errorHandler) {}

std::vector<std::unique_ptr<Stmt>> Parser::parse() {
  std::vector<std::unique_ptr<Stmt>> stmts;

  while (!isAtEnd()) {
    try {
      stmts.push_back(declaration(true));
    } catch (const ParseError &e) {
      synchronize();
    }
  }

  return stmts;
}

std::unique_ptr<Stmt> Parser::declaration(bool fileScope) {
  std::vector<Token> qualifiers = parseQualifiers();
  Token name = consume(TokenType::IDENTIFIER,
                       "Expected identifier in declaration.");
  if (peek().type == TokenType::LEFT_PAREN) {
    return function(fileScope, name, qualifiers);
  }

  return varDeclaration(fileScope, false, name, qualifiers);
}

std::unique_ptr<Stmt> Parser::function(bool fileScope, Token name, const std::vector<Token>& qualifiers) {
  consume(TokenType::LEFT_PAREN, "expect '(' after function name.");

  std::vector<std::unique_ptr<Stmt>> params;
  bool void_in_params = false;
  if (!check(TokenType::RIGHT_PAREN)) {
    do {
      if (match({TokenType::VOID})) {
        if (void_in_params) {
          error(peek(), "void repeated in function parameter list");
        } else if (!params.empty()) {
          error(peek(), "void appears with other parameters");
        }

        void_in_params = true;
      } else {
        // can't intermix void and typed parameters
        if (void_in_params) {
          error(peek(), "Function parameter contains void and typed parameters.");
        }

        Token type = consume(TokenType::INT, "Expected type for parameter.");
        Token name = consume(TokenType::IDENTIFIER, "Expected parameter name.");

        params.push_back(std::make_unique<Stmt>(FunctionParam(type, name)));
      }
    } while (match({TokenType::COMMA}));
  }

  consume(TokenType::RIGHT_PAREN, "Expected ')' after parameters.");
  
  Scope::StorageClass storageClass = getStorageClass(true, qualifiers);
  // function declaration
  if (match({TokenType::SEMICOLON})) {
    return std::make_unique<Stmt>(Function(fileScope, getType(qualifiers),
      storageClass, name, std::move(params), nullptr));
  } else {
    // function definition
    consume(TokenType::LEFT_BRACE, "Expected '{' before function body.");
    auto body = blockStatement();
    return std::make_unique<Stmt>(Function(fileScope, getType(qualifiers),
      storageClass, name, std::move(params), std::move(body)));
  }
}

std::unique_ptr<Stmt> Parser::varDeclaration(
  bool fileScope, bool loopDecl, Token name,
  const std::vector<Token>& qualifiers) {
  std::unique_ptr<Expr> init;
  if (match({TokenType::EQUAL})) {
    init = expression();
  }

  consume(TokenType::SEMICOLON, "expect ';' after var declaration.");
  Scope::StorageClass storageClass =
    getStorageClass(false, qualifiers);
  return std::make_unique<Stmt>(
    Decl(fileScope, loopDecl, getType(qualifiers), storageClass,
      std::make_unique<Expr>(Variable(name)), std::move(init)));
}

std::unique_ptr<Stmt> Parser::statement() {
  switch (peek().type) {
  case TokenType::WHILE:
    match({TokenType::WHILE});
    return whileStatement();
  case TokenType::DO:
    match({TokenType::DO});
    return doWhileStatement();
  case TokenType::FOR:
    match({TokenType::FOR});
    return forStatement();
  case TokenType::BREAK:
    match({TokenType::BREAK});
    consume(TokenType::SEMICOLON, "Expected ';' after break.");
    return std::make_unique<Stmt>(Break(previous()));
  case TokenType::CONTINUE:
    match({TokenType::CONTINUE});
    consume(TokenType::SEMICOLON, "Expected ';' after continue.");
    return std::make_unique<Stmt>(Continue(previous()));
  case TokenType::LEFT_BRACE:
    match({TokenType::LEFT_BRACE});
    return blockStatement();
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

  return std::make_unique<Stmt>(If(std::move(condition), std::move(thenBranch), std::move(elseBranch)));
}

std::unique_ptr<Stmt> Parser::whileStatement() {
  consume(TokenType::LEFT_PAREN, "need '(' in condition for while");
  auto condition = expression();
  consume(TokenType::RIGHT_PAREN, "need ')' in condition for while");

  auto body = statement();
  return std::make_unique<Stmt>(While(std::move(condition), std::move(body)));
}

std::unique_ptr<Stmt> Parser::doWhileStatement() {
  // do
  //  statement
  // while (condition);
  auto body = statement();
  consume(TokenType::WHILE, "Expected while in do ... while");
  consume(TokenType::LEFT_PAREN, "Expected '(' in condition for while");
  auto condition = expression();
  consume(TokenType::RIGHT_PAREN, "Expected ')' in condition for while");
  consume(TokenType::SEMICOLON, "Expected ';' after do .. while");

  return std::make_unique<Stmt>(DoWhile(std::move(body), std::move(condition)));
}

std::unique_ptr<Stmt> Parser::forStatement() {
  // for (init; condition; post)
  //   body
  consume(TokenType::LEFT_PAREN, "Expected '(' after for");

  // init can be a declaration or statment
  std::unique_ptr<Stmt> init = nullptr;
  if (isDeclarationFirstSet()) {
    std::vector<Token> qualifiers = parseQualifiers();
    Token name = consume(TokenType::IDENTIFIER, "Expected identifier after type.");
    init = varDeclaration(false, true, name, qualifiers);
  } else if (!check(TokenType::SEMICOLON)) {
    init = expressionStatement();
  } else {
    consume(TokenType::SEMICOLON, "Expected ';' in init");
  }

  // condition can be missing. if missing, it should default to true.
  std::unique_ptr<Expr> condition = nullptr;
  if (!check(TokenType::SEMICOLON)) {
    condition = expression();
  }
  consume(TokenType::SEMICOLON, "Expected ';' after condition");

  // post expression
  std::unique_ptr<Expr> post = nullptr;
  if (!check(TokenType::RIGHT_PAREN)) {
    post = expression();
  }
  consume(TokenType::RIGHT_PAREN, "Expected ')' after update");

  auto body = statement();
  body = std::make_unique<Stmt>(For(std::move(init), std::move(condition), std::move(post), std::move(body)));
  return body;
}

std::unique_ptr<Stmt> Parser::blockStatement() {
  std::vector<std::unique_ptr<Stmt>> stmts;

  while (!check(TokenType::RIGHT_BRACE) and !isAtEnd()) {
    if (isDeclarationFirstSet()) {
      stmts.emplace_back(declaration(false));
    } else {
      stmts.emplace_back(statement());
    }
  }

  consume(TokenType::RIGHT_BRACE, "Expected '}' after block");
  return std::make_unique<Stmt>(Block(std::move(stmts)));
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
    } else {
      break;
    }
  }

  return e;
}

std::unique_ptr<Expr> Parser::finishCall(std::unique_ptr<Expr> e) {
  std::vector<std::unique_ptr<Expr>> args;
  if (!check(TokenType::RIGHT_PAREN)) {
    do {
      args.push_back(expression());
    } while (match({TokenType::COMMA}));
  }

  Token paren = consume(TokenType::RIGHT_PAREN, "expected ')' in call");
  return std::make_unique<Expr>(Call(std::move(e), std::move(args)));
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

Token Parser::peek() const { return tokens_[current]; }

bool Parser::isAtEnd() const { return peek().type == TokenType::END_OF_FILE; }

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

std::vector<Token> Parser::parseQualifiers() {
  std::vector<Token> qualifiers;
  while (isDeclarationFirstSet()) {
    qualifiers.emplace_back(advance());
  }

  if (qualifiers.empty()) {
    error(peek(), "Expected qualifier in declaration.");
  } else {
    bool type_qualifier = false;
    bool storage_qualifier = false;
    for (Token& tok : qualifiers) {
      if (isStorageQualifier(tok)) {
        if (storage_qualifier) {
          error(tok, "Multiple storage qualifiers in declaration.");
        }

        storage_qualifier = true;
      }

      if (tok.type == TokenType::INT) {
        type_qualifier = true;
      }
    }

    if (!type_qualifier) {
      error(peek(), "Expected type qualifier in declaration.");
    }
  }

  return qualifiers;
}

bool Parser::isDeclarationFirstSet() const {
  static std::vector<TokenType> firstSet =
    {TokenType::INT, TokenType::STATIC, TokenType::EXTERN};

  if (!isAtEnd()) {
    auto it = std::find(firstSet.begin(), firstSet.end(), peek().type);
    return it != firstSet.end();
  }

  return false;
}

bool Parser::isTypeQualifier(const Token& tok) const {
  return (tok.type == TokenType::INT);
}

Token Parser::getType(const std::vector<Token>& qualifiers) const {
  for (const Token& tok : qualifiers) {
    if (isTypeQualifier(tok)) {
      return tok;
    }
  }

  assert(0);
}

Scope::StorageClass Parser::getStorageClass(
  bool isFunction, const std::vector<Token>& qualifiers) const {
  // Functions have extern storage class by default. Variables have default
  // storage class of auto even if they are defined at file scope. If qualifier
  // list contains a qualification, pick that.
  Scope::StorageClass storageClass =
    isFunction ? Scope::STORAGE_EXTERN : Scope::STORAGE_AUTO;
  Scope::StorageClass computedClass = getStorageClass(qualifiers);
  return (computedClass != Scope::STORAGE_AUTO) ? computedClass : storageClass;
}

Scope::StorageClass Parser::getStorageClass(
  const std::vector<Token>& qualifiers) const {
  for (const Token& tok : qualifiers) {
    if (isStorageQualifier(tok)) {
      return (tok.type == TokenType::EXTERN) ?
              Scope::STORAGE_EXTERN : Scope::STORAGE_STATIC;
    }
  }

  return Scope::STORAGE_AUTO;
}
