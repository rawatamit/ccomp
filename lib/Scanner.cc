#include "Scanner.h"
#include "ErrorHandler.h"

using namespace ccomp;

Scanner::Scanner(const std::string &aSource, ErrorHandler &aErrorHandler)
    : start(0), current(0), line(1), source(aSource),
      errorHandler(aErrorHandler) {
  // initialize reserved keywords map
  reservedKeywords["int"] = TokenType::INT;
  reservedKeywords["void"] = TokenType::VOID;
  reservedKeywords["return"] = TokenType::RETURN;
  #if 1
  reservedKeywords["and"] = TokenType::AND;
  reservedKeywords["class"] = TokenType::CLASS;
  reservedKeywords["else"] = TokenType::ELSE;
  reservedKeywords["false"] = TokenType::FALSE;
  reservedKeywords["for"] = TokenType::FOR;
  reservedKeywords["fun"] = TokenType::FUN;
  reservedKeywords["if"] = TokenType::IF;
  reservedKeywords["nil"] = TokenType::NIL;
  reservedKeywords["or"] = TokenType::OR;
  reservedKeywords["print"] = TokenType::PRINT;
  reservedKeywords["return"] = TokenType::RETURN;
  reservedKeywords["super"] = TokenType::SUPER;
  reservedKeywords["this"] = TokenType::THIS;
  reservedKeywords["true"] = TokenType::TRUE;
  reservedKeywords["var"] = TokenType::VAR;
  reservedKeywords["while"] = TokenType::WHILE;
  #endif
}

char Scanner::advanceAndGetChar() {
  ++current;
  return source[current - 1];
}

bool Scanner::isWhitespace(char c) {
  switch (c) {
  case ' ':
  case '\r':
  case '\t':
  case '\n':
    return true;
  default:
    return false;
  }
}

void Scanner::scanAndAddToken() {
  const char c = advanceAndGetChar();
  switch (c) {
  case '(':
    addToken(TokenType::LEFT_PAREN);
    break;
  case ')':
    addToken(TokenType::RIGHT_PAREN);
    break;
  case '{':
    addToken(TokenType::LEFT_BRACE);
    break;
  case '}':
    addToken(TokenType::RIGHT_BRACE);
    break;
  case ',':
    addToken(TokenType::COMMA);
    break;
  case '.':
    addToken(TokenType::DOT);
    break;
  case '+':
    addToken(TokenType::PLUS);
    break;
  case ';':
    addToken(TokenType::SEMICOLON);
    break;
  case '*':
    addToken(TokenType::STAR);
    break;
  case '~':
    addToken(TokenType::TILDE);
    break;
  case '-':
    addToken(matchAndAdvance('-') ? TokenType::MINUS_MINUS : TokenType::MINUS);
    break;
  case '!':
    addToken(matchAndAdvance('=') ? TokenType::BANG_EQUAL : TokenType::BANG);
    break;
  case '=':
    addToken(matchAndAdvance('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL);
    break;
  case '<':
    addToken(matchAndAdvance('=') ? TokenType::LESS_EQUAL : TokenType::LESS);
    break;
  case '>':
    addToken(matchAndAdvance('=') ? TokenType::GREATER_EQUAL
                                  : TokenType::GREATER);
    break;
  case '/':
    if (matchAndAdvance('/')) {
      // single line comment
      while (peek() != '\n' && !isAtEnd())
        (void)advanceAndGetChar();
    } else if (matchAndAdvance('*')) {
      // multi line comment
      char cur = advanceAndGetChar();
      while (!isAtEnd()) {
        // end of multi line comment
        if (cur == '*' && matchAndAdvance('/')) {
          break;
        }
        cur = advanceAndGetChar();
      }
      // skip /
      // cur = advanceAndGetChar();
    } else {
      addToken(TokenType::SLASH);
    }
    break;
  case '"':
    string();
    break;
  default:
    if (isWhitespace(c)) {
      // ignore whitespace
      line += 1 ? (c == '\n') : 0;
      break;
    } else if (isDigit(c)) {
      number();
    } else if (isAlpha(c)) {
      identifier();
    } else {
      std::string errorMessage = "Unexpected character: '";
      errorMessage += c;
      errorMessage += "'.";
      errorHandler.add(line, "", errorMessage);
      break;
    }
  }
}

bool Scanner::isAlpha(const char c) const {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Scanner::isAlphaNumeric(const char c) const {
  return isAlpha(c) || isDigit(c);
}

void Scanner::identifier() {
  // using "maximal munch"
  // e.g. match "orchid" not "or" keyword and "chid"
  while (isAlphaNumeric(peek()))
    (void)advanceAndGetChar();
  // see if the identifier is a reserved keyword
  const size_t identifierLength = current - start;
  const std::string identifier = source.substr(start, identifierLength);
  const bool isReservedKeyword =
      reservedKeywords.find(identifier) != reservedKeywords.end();
  if (isReservedKeyword) {
    addToken(reservedKeywords[identifier]);
  } else {
    addToken(TokenType::IDENTIFIER);
  }
}

bool Scanner::isDigit(const char c) const { return c >= '0' && c <= '9'; }

void Scanner::number() {
  while (isDigit(peek()))
    (void)advanceAndGetChar();
  // look for fractional part
  if (peek() == '.' && isDigit(peekNext())) {
    // consume the "."
    (void)advanceAndGetChar();
    while (isDigit(peek()))
      (void)advanceAndGetChar();
  } else if (isAlpha(peek())) {
    // malformed number.
    const size_t numberLength = current - start;
    std::string errorMessage = "Malformed number: ";
    errorMessage += source.substr(start, numberLength);;
    errorMessage += ".";
    errorHandler.add(line, "", errorMessage);
    return;
  }
  const size_t numberLength = current - start;
  const std::string numberLiteral = source.substr(start, numberLength);
  addToken(TokenType::NUMBER, numberLiteral);
}

void Scanner::string() {
  while (peek() != '"' && !isAtEnd()) {
    if (peek() == '\n')
      ++line;
    (void)advanceAndGetChar();
  }
  // unterminated string
  if (isAtEnd()) {
    errorHandler.add(line, "", "Unterminated string.");
    return;
  }
  // closing "
  (void)advanceAndGetChar();
  const size_t stringSize = current - start;
  // trim the surrounding quotes
  const std::string stringLiteral = source.substr(start + 1, stringSize - 2);
  addToken(TokenType::STRING, stringLiteral);
}

void Scanner::addToken(const TokenType aTokenType, const std::string &value) {
  const size_t lexemeSize = current - start;
  const auto lexeme = source.substr(start, lexemeSize);
  tokens.push_back(Token(aTokenType, lexeme, value, line));
}

void Scanner::addToken(const TokenType aTokenType) { addToken(aTokenType, ""); }

bool Scanner::isAtEnd() const { return current >= source.size(); }

bool Scanner::matchAndAdvance(const char aExpected) {
  if (isAtEnd())
    return false;
  if (source[current] != aExpected)
    return false;
  ++current;
  return true;
}

char Scanner::peekNext() const {
  if (current + 1 >= source.length())
    return '\0';
  return source[current + 1];
}

char Scanner::peek() const {
  if (isAtEnd())
    return '\0';
  return source[current];
}

std::vector<Token> Scanner::scanAndGetTokens() {
  while (!isAtEnd()) {
    // we are at the beginning of the next lexeme
    start = current;
    scanAndAddToken();
  }
  tokens.push_back(Token(TokenType::END_OF_FILE, "", "", line));
  return tokens;
}
