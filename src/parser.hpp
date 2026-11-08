#pragma once

#include <memory>
#include <string>
#include <vector>

namespace flux {

namespace Parser {

enum class ParserErrorType {
  UnexpectedToken,
  UnexpectedEndOfFile,
  InvalidSyntax,
  InvalidExpression,
  InvalidStatement,
  InvalidFunctionDefinition,
  InvalidVariableDeclaration,
  InvalidTypeDefinition,
  InvalidImportStatement,
  InvalidIfStatement,
  InvalidWhileStatement,
  InvalidForStatement,
  InvalidMatchStatement,
  InvalidCaseStatement,
  InvalidBreakStatement,
  InvalidContinueStatement,
  InvalidReturnStatement,
  InvalidYieldStatement,
  InvalidDeferStatement,
  InvalidAcceptStatement,
  InvalidResumeStatement,
  InvalidImmediateStatement
};

struct ParserResult {
  bool success;
  std::string errorMessage;
  ParserErrorType errorType;

  ParserResult(bool s, const std::string& msg = "", ParserErrorType type = ParserErrorType::UnexpectedToken)
      : success(s), errorMessage(msg), errorType(type) {}
};

namespace Token {

enum class TokenType {
  Identifier,
  Number,
  String,
  Keyword,
  Symbol,
  EndOfFile,
};

class Token {
 public:
  Token(TokenType type, const std::string& value, int line, int column)
      : _type(type), _value(value), _line(line), _column(column) {}

  TokenType getType() const { return _type; }
  const std::string& getValue() const { return _value; }
  int getLine() const { return _line; }
  int getColumn() const { return _column; }

 private:
  TokenType _type;
  std::string _value;
  int _line;
  int _column;
};

enum class KeywordType {
  Def,
  Let,
  Var,
  Typedef,
  Import,
  If,
  Else,
  Elif,
  While,
  For,
  Match,
  Case,
  Break,
  Continue,
  Return,
  Yield,
  Defer,
  Accept,
  Resume,
  Immediate,
};

ParserResult tokenize(const std::string& source, std::vector<Token>& tokens);

}  // namespace Token

enum ASTNodeType {
  Statement,
  Expression,
  FunctionDefinition,
  VariableDeclaration,
  TypeDefinition,
  ImportStatement,
  IfStatement,
  WhileStatement,
  ForStatement,
  MatchStatement,
  CaseStatement,
  BreakStatement,
  ContinueStatement,
  ReturnStatement,
  YieldStatement,
  DeferStatement,
  AcceptStatement,
  ResumeStatement,
  ImmediateStatement,
  UnaryExpression,
  BinaryExpression,
  LiteralExpression,
  IdentifierExpression,
  CallExpression,
  AssignmentExpression,
};

enum class ModifierType {
  PASSIVE,
  ACTIVE,
  MUTABLE,
  IMMUTABLE,
  INT,
  FLOAT,
  DOUBLE,
  CHAR,
  STRING,
  FORMATTED_STRING,
  BOOL,
  VOID,
};

struct Parameter {
  std::string name;
  std::vector<ModifierType> modifiers;
};

class ASTNode {
 public:
  virtual ~ASTNode() = default;
  ASTNodeType type;
};

class ExpressionNode : public ASTNode {
 public:
  virtual ~ExpressionNode() = default;
  std::unique_ptr<ExpressionNode> left;
  std::unique_ptr<ExpressionNode> right;
};

class SingleExpressionNode : public ExpressionNode {
 public:
  virtual ~SingleExpressionNode() = default;
  std::string value;
};

class StatementNode : public ASTNode {
 public:
  virtual ~StatementNode() = default;
  std::unique_ptr<ExpressionNode> expr;
  std::unique_ptr<StatementNode> body;
  std::unique_ptr<StatementNode> next;
};

class StatementListNode : public StatementNode {
 public:
  virtual ~StatementListNode() = default;
  std::vector<std::unique_ptr<StatementNode>> statements;
};

class FunctionDefinitionNode : public StatementNode {
 public:
  virtual ~FunctionDefinitionNode() = default;
  std::string name;
  std::vector<ModifierType> function_modifiers;
  std::vector<Parameter> parameters;
  std::unique_ptr<StatementListNode> body;
};

class VariableDeclarationNode : public StatementNode {
 public:
  virtual ~VariableDeclarationNode() = default;
  std::string name;
  std::vector<ModifierType> variable_modifiers;
  std::unique_ptr<ExpressionNode> initializer;
};

ParserResult parse(const std::vector<Token::Token>& tokens, std::unique_ptr<ASTNode>& ast);

}  // namespace Parser

}  // namespace flux
