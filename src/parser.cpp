#include "parser.hpp"

#include <cctype>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace flux::Parser {

ParserResult syntaxError(const std::string& message, ParserErrorType type,
                         int line, int column) {
  std::ostringstream result;
  result << "line " << line << ", column " << column << ": " << message;
  return ParserResult(false, result.str(), type);
}

bool isIdentifierStart(char value) {
  const unsigned char character = static_cast<unsigned char>(value);
  return std::isalpha(character) || value == '_';
}

bool isIdentifierPart(char value) {
  const unsigned char character = static_cast<unsigned char>(value);
  return std::isalnum(character) || value == '_';
}

bool isTypeKeyword(const std::string& value) {
  return value == "char" || value == "short" || value == "int" ||
         value == "long" || value == "float" || value == "double" ||
         value == "bool" || value == "string";
}

bool isModifierKeyword(const std::string& value) {
  return value == "passive" || value == "active" || value == "immediate" ||
         value == "mutable" || value == "immutable";
}

ModifierType modifierFromName(const std::string& value) {
  if (value == "passive") return ModifierType::PASSIVE;
  if (value == "active") return ModifierType::ACTIVE;
  if (value == "immediate") return ModifierType::IMMEDIATE;
  if (value == "mutable") return ModifierType::MUTABLE;
  if (value == "immutable") return ModifierType::IMMUTABLE;
  if (value == "char") return ModifierType::CHAR;
  if (value == "short") return ModifierType::SHORT;
  if (value == "int") return ModifierType::INT;
  if (value == "long") return ModifierType::LONGLONG;
  if (value == "float") return ModifierType::FLOAT;
  if (value == "double") return ModifierType::DOUBLE;
  if (value == "bool") return ModifierType::BOOL;
  if (value == "string") return ModifierType::STRING;
  return ModifierType::CUSTOM;
}

class ParseFailure : public std::runtime_error {
 public:
  ParseFailure(ParserErrorType type, const std::string& message, int line,
               int column)
      : std::runtime_error(message), type(type), line(line), column(column) {}

  ParserErrorType type;
  int line;
  int column;
};

class ParserImpl {
 public:
  explicit ParserImpl(const std::vector<Token>& tokens) : tokens_(tokens) {}

  std::unique_ptr<ProgramNode> parseProgram() {
    auto program = std::make_unique<ProgramNode>();
    program->body = std::make_unique<StatementListNode>();

    while (!atEnd()) {
      if (isSymbol("NEWLINE")) {
        advance();
        continue;
      }
      if (isSymbol("INDENT") || isSymbol("DEDENT")) {
        fail("unexpected indentation", ParserErrorType::InvalidSyntax);
      }
      program->body->statements.push_back(parseStatement());
    }

    return program;
  }

 private:
  const std::vector<Token>& tokens_;
  std::size_t position_ = 0;

  const Token& current() const {
    return tokens_[position_ < tokens_.size() ? position_ : tokens_.size() - 1];
  }

  const Token& peek(std::size_t offset = 1) const {
    const std::size_t index = position_ + offset;
    return tokens_[index < tokens_.size() ? index : tokens_.size() - 1];
  }

  bool atEnd() const { return current().getType() == TokenType::EndOfFile; }

  bool isSymbol(const std::string& value) const {
    return current().getType() == TokenType::Symbol &&
           current().getValue() == value;
  }

  bool isKeyword(const std::string& value) const {
    return current().getType() == TokenType::Keyword &&
           current().getValue() == value;
  }

  const Token& advance() {
    const Token& token = current();
    if (!atEnd()) ++position_;
    return token;
  }

  bool matchSymbol(const std::string& value) {
    if (!isSymbol(value)) return false;
    advance();
    return true;
  }

  bool matchKeyword(const std::string& value) {
    if (!isKeyword(value)) return false;
    advance();
    return true;
  }

  [[noreturn]] void fail(const std::string& message,
                         ParserErrorType type = ParserErrorType::UnexpectedToken) const {
    throw ParseFailure(type, message, current().getLine(),
                       current().getColumn());
  }

  const Token& expectSymbol(const std::string& value) {
    if (!isSymbol(value)) {
      fail("expected '" + value + "'");
    }
    return advance();
  }

  const Token& expectIdentifier() {
    if (current().getType() != TokenType::Identifier) {
      fail("expected an identifier");
    }
    return advance();
  }

  void expectLineEnd() {
    if (!matchSymbol("NEWLINE") && !atEnd() && !isSymbol("DEDENT")) {
      fail("expected the end of the line");
    }
  }

  std::unique_ptr<StatementNode> parseStatement() {
    if (matchKeyword("if")) return parseIfStatement();
    if (matchKeyword("while")) return parseWhileStatement();
    if (matchKeyword("for")) return parseForStatement();
    if (matchKeyword("match")) return parseMatchStatement();
    if (matchKeyword("def")) return parseFunctionDefinition();
    if (isKeyword("let") || isKeyword("var")) return parseVariableDeclaration();
    if (matchKeyword("return")) return parseReturnStatement();
    if (matchKeyword("break")) return parseLoopControl(BreakStatement);
    if (matchKeyword("continue")) return parseLoopControl(ContinueStatement);
    if (matchKeyword("print")) return parsePrintStatement();
    if (matchKeyword("pass")) {
      auto statement = std::make_unique<StatementNode>();
      expectLineEnd();
      return statement;
    }

    auto expression = parseExpression();
    if (matchSymbol("=")) {
      auto assignment = std::make_unique<ExpressionNode>();
      assignment->type = BinaryExpression;
      assignment->op = "=";
      assignment->left = std::move(expression);
      assignment->right = parseExpression();
      expression = std::move(assignment);
    }
    auto statement = std::make_unique<StatementNode>();
    statement->type = Statement;
    statement->expr = std::move(expression);
    expectLineEnd();
    return statement;
  }

  std::unique_ptr<StatementListNode> parseSuite() {
    expectLineEnd();
    expectSymbol("INDENT");

    auto statements = std::make_unique<StatementListNode>();
    while (!atEnd() && !isSymbol("DEDENT")) {
      if (isSymbol("NEWLINE")) {
        advance();
        continue;
      }
      if (isSymbol("INDENT")) fail("unexpected indentation");
      statements->statements.push_back(parseStatement());
    }

    if (statements->statements.empty()) {
      fail("an indented block must contain a statement",
           ParserErrorType::InvalidSyntax);
    }
    expectSymbol("DEDENT");
    return statements;
  }

  std::unique_ptr<IfStatementNode> parseIfStatement() {
    auto statement = std::make_unique<IfStatementNode>();
    statement->condition = parseExpression();
    expectSymbol(":");
    statement->body = parseSuite();

    while (matchKeyword("elif")) {
      if (isSymbol(":")) {
        statement->else_body = parseSuiteAfterColon();
        return statement;
      }
      auto branch = std::make_unique<IfStatementNode>();
      branch->condition = parseExpression();
      expectSymbol(":");
      branch->body = parseSuite();
      statement->elif_branches.push_back(std::move(branch));
    }
    if (matchKeyword("else")) {
      expectSymbol(":");
      statement->else_body = parseSuite();
    }
    return statement;
  }

  std::unique_ptr<StatementListNode> parseSuiteAfterColon() {
    expectSymbol(":");
    return parseSuite();
  }

  std::unique_ptr<WhileStatementNode> parseWhileStatement() {
    auto statement = std::make_unique<WhileStatementNode>();
    statement->condition = parseExpression();
    expectSymbol(":");
    statement->body = parseSuite();
    return statement;
  }

  std::unique_ptr<ForStatementNode> parseForStatement() {
    auto statement = std::make_unique<ForStatementNode>();
    const Token& name = expectIdentifier();
    statement->element = std::make_unique<IdentifierExpressionNode>();
    statement->element->name = name.getValue();
    if (!matchKeyword("in")) fail("expected 'in' in for statement");
    statement->iterator = parseExpression();
    expectSymbol(":");
    statement->body = parseSuite();
    return statement;
  }

  std::unique_ptr<MatchStatementNode> parseMatchStatement() {
    auto statement = std::make_unique<MatchStatementNode>();
    statement->subject = parseExpression();
    expectSymbol(":");
    expectLineEnd();
    expectSymbol("INDENT");

    while (!atEnd() && !isSymbol("DEDENT")) {
      if (!matchKeyword("case")) {
        fail("expected 'case' in match statement");
      }
      auto caseStatement = std::make_unique<CaseStatementNode>();
      caseStatement->pattern = parseExpression();
      expectSymbol(":");
      caseStatement->body = parseSuite();
      statement->cases.push_back(std::move(caseStatement));
    }

    if (statement->cases.empty()) {
      fail("match statement must contain at least one case",
           ParserErrorType::InvalidSyntax);
    }
    expectSymbol("DEDENT");
    return statement;
  }

  std::unique_ptr<FunctionDefinitionNode> parseFunctionDefinition() {
    auto statement = std::make_unique<FunctionDefinitionNode>();
    while (isModifierKeyword(current().getValue()) ||
           isTypeKeyword(current().getValue())) {
      statement->function_modifiers.insert(
          modifierFromName(advance().getValue()));
    }
    statement->name = expectIdentifier().getValue();
    expectSymbol("(");

    if (!isSymbol(")")) {
      do {
        statement->parameter_types.push_back(parseType());
        statement->parameter_names.push_back(expectIdentifier().getValue());
      } while (matchSymbol(","));
    }
    expectSymbol(")");
    expectSymbol(":");
    statement->body = parseSuite();
    return statement;
  }

  std::unique_ptr<TypeModifierNode> parseType() {
    auto type = std::make_unique<BasicTypeNode>();
    while (isModifierKeyword(current().getValue())) {
      auto modifier = std::make_unique<BasicTypeNode>();
      modifier->basic_type = modifierFromName(advance().getValue());
      modifier->name = tokens_[position_ - 1].getValue();
      type->sub_types.push_back(std::move(modifier));
    }

    if (current().getType() != TokenType::Keyword &&
        current().getType() != TokenType::Identifier) {
      fail("expected a type name");
    }
    const std::string typeName = advance().getValue();
    if (!isTypeKeyword(typeName) &&
        tokens_[position_ - 1].getType() != TokenType::Identifier) {
      fail("expected a type name");
    }
    type->basic_type = modifierFromName(typeName);
    type->name = typeName;
    return type;
  }

  std::unique_ptr<VariableDeclarationNode> parseVariableDeclaration() {
    const std::string declarationKind = advance().getValue();
    auto statement = std::make_unique<VariableDeclarationNode>();
    statement->is_mutable = declarationKind == "var";

    while (isModifierKeyword(current().getValue()) ||
           isTypeKeyword(current().getValue())) {
      statement->variable_modifiers.insert(
          modifierFromName(advance().getValue()));
    }
    if (current().getType() == TokenType::Identifier &&
        peek().getType() == TokenType::Identifier) {
      statement->variable_modifiers.insert(ModifierType::CUSTOM);
      advance();
    }
    statement->name = expectIdentifier().getValue();
    if (!matchSymbol("=")) {
      fail("expected '=' in " + declarationKind + " declaration");
    }
    statement->initializer = parseExpression();
    expectLineEnd();
    return statement;
  }

  std::unique_ptr<ReturnStatementNode> parseReturnStatement() {
    auto statement = std::make_unique<ReturnStatementNode>();
    if (!isSymbol("NEWLINE") && !isSymbol("DEDENT") && !atEnd()) {
      statement->expr = parseExpression();
    }
    expectLineEnd();
    return statement;
  }

  std::unique_ptr<StatementNode> parseLoopControl(ASTNodeType type) {
    auto statement = std::make_unique<StatementNode>();
    statement->type = type;
    expectLineEnd();
    return statement;
  }

  std::unique_ptr<StatementNode> parsePrintStatement() {
    auto statement = std::make_unique<StatementNode>();
    statement->type = PrintStatement;
    if (!isSymbol("NEWLINE")) statement->expr = parseExpression();
    expectLineEnd();
    return statement;
  }

  std::unique_ptr<ExpressionNode> parseExpression(int minimumPrecedence = 1) {
    auto left = parseUnary();
    while (true) {
      const int precedence = binaryPrecedence(current().getValue());
      if (precedence < minimumPrecedence) break;
      const std::string operation = advance().getValue();
      auto right = parseExpression(precedence + 1);
      auto binary = std::make_unique<ExpressionNode>();
      binary->type = BinaryExpression;
      binary->op = operation;
      binary->left = std::move(left);
      binary->right = std::move(right);
      left = std::move(binary);
    }
    return left;
  }

  static int binaryPrecedence(const std::string& operation) {
    if (operation == "||") return 1;
    if (operation == "&&") return 2;
    if (operation == "==" || operation == "!=") return 3;
    if (operation == "<" || operation == "<=" || operation == ">" ||
        operation == ">=") {
      return 4;
    }
    if (operation == "+" || operation == "-") return 5;
    if (operation == "*" || operation == "/" || operation == "%") return 6;
    return 0;
  }

  std::unique_ptr<ExpressionNode> parseUnary() {
    if (isSymbol("!") || isSymbol("-") || isSymbol("+")) {
      const std::string operation = advance().getValue();
      auto expression = std::make_unique<ExpressionNode>();
      expression->type = UnaryExpression;
      expression->op = operation;
      expression->right = parseUnary();
      return expression;
    }
    return parsePostfix();
  }

  std::unique_ptr<ExpressionNode> parsePostfix() {
    auto expression = parsePrimary();
    while (matchSymbol("(")) {
      auto call = std::make_unique<CallExpressionNode>();
      auto* identifier = dynamic_cast<IdentifierExpressionNode*>(expression.get());
      if (identifier == nullptr) {
        fail("only named functions can be called");
      }
      call->function_name = identifier->name;
      if (!isSymbol(")")) {
        do {
          call->arguments.push_back(parseExpression());
        } while (matchSymbol(","));
      }
      expectSymbol(")");
      expression = std::move(call);
    }
    return expression;
  }

  std::unique_ptr<ExpressionNode> parsePrimary() {
    if (current().getType() == TokenType::Number ||
        current().getType() == TokenType::String || isKeyword("true") ||
        isKeyword("false")) {
      auto expression = std::make_unique<LiteralExpressionNode>();
      expression->value = advance().getValue();
      return expression;
    }
    if (current().getType() == TokenType::Identifier) {
      auto expression = std::make_unique<IdentifierExpressionNode>();
      expression->name = advance().getValue();
      return expression;
    }
    if (matchSymbol("(")) {
      auto expression = parseExpression();
      expectSymbol(")");
      return expression;
    }
    fail("expected an expression",
         atEnd() ? ParserErrorType::UnexpectedEndOfFile
                 : ParserErrorType::UnexpectedToken);
  }
};

ParserResult tokenize(const std::string& source,
                             std::vector<Token>& tokens) {
  std::vector<Token> result;
  std::vector<std::size_t> indentation{0};
  std::size_t offset = 0;
  int lineNumber = 1;

  while (offset < source.size()) {
    const std::size_t lineStart = offset;
    while (offset < source.size() && source[offset] != '\n') ++offset;
    const std::size_t lineEnd = offset;
    if (offset < source.size()) ++offset;

    std::size_t cursor = lineStart;
    std::size_t indent = 0;
    while (cursor < lineEnd && (source[cursor] == ' ' || source[cursor] == '\t')) {
      if (source[cursor] == '\t') {
        return syntaxError("tabs are not allowed for indentation",
                           ParserErrorType::InvalidSyntax, lineNumber,
                           static_cast<int>(cursor - lineStart + 1));
      }
      ++indent;
      ++cursor;
    }
    if (cursor == lineEnd || source[cursor] == '#') {
      ++lineNumber;
      continue;
    }

    if (indent > indentation.back()) {
      indentation.push_back(indent);
      result.emplace_back(TokenType::Symbol, "INDENT", lineNumber, 1);
    } else {
      while (indent < indentation.back()) {
        indentation.pop_back();
        result.emplace_back(TokenType::Symbol, "DEDENT", lineNumber,
                            static_cast<int>(cursor - lineStart + 1));
      }
      if (indent != indentation.back()) {
        return syntaxError("inconsistent indentation",
                           ParserErrorType::InvalidSyntax, lineNumber,
                           static_cast<int>(cursor - lineStart + 1));
      }
    }

    while (cursor < lineEnd) {
      const char character = source[cursor];
      if (std::isspace(static_cast<unsigned char>(character))) {
        ++cursor;
        continue;
      }
      if (character == '#') break;

      const int column = static_cast<int>(cursor - lineStart + 1);
      if (isIdentifierStart(character)) {
        const std::size_t start = cursor++;
        while (cursor < lineEnd && isIdentifierPart(source[cursor])) ++cursor;
        const std::string value = source.substr(start, cursor - start);
        result.emplace_back(keywords.count(value) ? TokenType::Keyword
                                                   : TokenType::Identifier,
                            value, lineNumber, column);
        continue;
      }
      if (std::isdigit(static_cast<unsigned char>(character))) {
        const std::size_t start = cursor++;
        while (cursor < lineEnd &&
               std::isdigit(static_cast<unsigned char>(source[cursor]))) {
          ++cursor;
        }
        if (cursor < lineEnd && source[cursor] == '.' &&
            cursor + 1 < lineEnd &&
            std::isdigit(static_cast<unsigned char>(source[cursor + 1]))) {
          ++cursor;
          while (cursor < lineEnd &&
                 std::isdigit(static_cast<unsigned char>(source[cursor]))) {
            ++cursor;
          }
        }
        if (cursor < lineEnd &&
            (source[cursor] == 'e' || source[cursor] == 'E')) {
          const std::size_t exponent = cursor++;
          if (cursor < lineEnd &&
              (source[cursor] == '+' || source[cursor] == '-')) {
            ++cursor;
          }
          const std::size_t digits = cursor;
          while (cursor < lineEnd &&
                 std::isdigit(static_cast<unsigned char>(source[cursor]))) {
            ++cursor;
          }
          if (digits == cursor) {
            return syntaxError("invalid numeric exponent",
                               ParserErrorType::InvalidSyntax, lineNumber,
                               static_cast<int>(exponent - lineStart + 1));
          }
        }
        result.emplace_back(TokenType::Number,
                            source.substr(start, cursor - start), lineNumber,
                            column);
        continue;
      }
      if (character == '"' || character == '\'') {
        const char quote = character;
        const std::size_t start = cursor++;
        bool closed = false;
        while (cursor < lineEnd) {
          if (source[cursor] == '\\') {
            cursor += cursor + 1 < lineEnd ? 2 : 1;
          } else if (source[cursor++] == quote) {
            closed = true;
            break;
          }
        }
        if (!closed) {
          return syntaxError("unterminated string literal",
                             ParserErrorType::InvalidSyntax, lineNumber,
                             column);
        }
        result.emplace_back(TokenType::String,
                            source.substr(start, cursor - start), lineNumber,
                            column);
        continue;
      }

      const std::string twoCharacter =
          cursor + 1 < lineEnd ? source.substr(cursor, 2) : std::string();
      if (twoCharacter == "==" || twoCharacter == "!=" ||
          twoCharacter == "<=" || twoCharacter == ">=" ||
          twoCharacter == "||" || twoCharacter == "&&") {
        result.emplace_back(TokenType::Symbol, twoCharacter, lineNumber,
                            column);
        cursor += 2;
        continue;
      }
      if (std::string("+-*/%=<>!()[]{}.,:").find(character) !=
          std::string::npos) {
        result.emplace_back(TokenType::Symbol, std::string(1, character),
                            lineNumber, column);
        ++cursor;
        continue;
      }
      return syntaxError(std::string("unexpected character '") + character +
                             "'",
                         ParserErrorType::InvalidSyntax, lineNumber, column);
    }

    result.emplace_back(TokenType::Symbol, "NEWLINE", lineNumber,
                        static_cast<int>(lineEnd - lineStart + 1));
    ++lineNumber;
  }

  while (indentation.size() > 1) {
    indentation.pop_back();
    result.emplace_back(TokenType::Symbol, "DEDENT", lineNumber, 1);
  }
  result.emplace_back(TokenType::EndOfFile, "", lineNumber, 1);
  tokens = std::move(result);
  return ParserResult(true);
}

ParserResult parse(const std::vector<Token>& tokens,
                   std::unique_ptr<ASTNode>& ast) {
  if (tokens.empty() || tokens.back().getType() != TokenType::EndOfFile) {
    return ParserResult(false, "token stream must end with an EOF token",
                        ParserErrorType::UnexpectedEndOfFile);
  }
  try {
    ParserImpl parser(tokens);
    std::unique_ptr<ProgramNode> program = parser.parseProgram();
    ast = std::move(program);
    return ParserResult(true);
  } catch (const ParseFailure& error) {
    return syntaxError(error.what(), error.type, error.line, error.column);
  }
}

}  // namespace flux::Parser
