#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_set>

namespace flux::Parser {

enum class ParserErrorType {
  UnexpectedToken,
  UnexpectedEndOfFile,
  InvalidSyntax,
  //...
};

struct ParserResult {
  bool success;
  std::string errorMessage;
  ParserErrorType errorType;

  ParserResult(bool s, const std::string& msg = "", ParserErrorType type = ParserErrorType::UnexpectedToken)
      : success(s), errorMessage(msg), errorType(type) {}
};

enum class TokenType {
  Identifier,
  Number,
  String,
  FormattedString,
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

static const std::unordered_set<std::string> keywords = {
    "def", "let", "var", "typedef", "import", "as",
    "if", "else", "elif", "while", "for", "in", "match", "case", "default", "break", "continue", "return",
    "defer", "immediate", "accept", "yield", "resume",
    "char", "short", "int", "long", "float", "double", "bool", "string",
    "passive", "active", "transient", "inline",
    "true", "false", "print", "pass",
    "nonlocal", "self", "fetch", "requiring", "template", "virtual",
    "try", "catch", "finally", "throw",
};

ParserResult tokenize(const std::string& source, std::vector<Token>& tokens);

enum ASTNodeType {
  Program,
  Statement,
  Expression,
  TypeModifier,
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
  WaitForYieldStatement,
  YieldStatement,
  DeferStatement,
  AcceptStatement,
  ResumeStatement,
  PrintStatement,
  UnaryExpression,
  BinaryExpression,
  LiteralExpression,
  IdentifierExpression,
  CallExpression,
  ArrayExpression,
  StructureExpression,
  FormattedStringExpression,
  MemberAccessExpression,
  ListComprehensionExpression,
};

enum class ModifierType {
  PASSIVE,
  ACTIVE,
  IMMEDIATE,
  CHAR,
  SHORT,
  INT,
  LONGLONG,
  FLOAT,
  DOUBLE,
  BOOL,
  STRING,
  CUSTOM,
};

class ASTNode {
 public:
  ASTNode() : type(Statement) {}
  virtual ~ASTNode() = default;
  ASTNodeType type;
};

class TypeModifierNode : public ASTNode {
 public:
  TypeModifierNode() { type = TypeModifier; }
  virtual ~TypeModifierNode() = default;
  std::vector<std::unique_ptr<TypeModifierNode>> sub_types;
};

class BasicTypeNode : public TypeModifierNode {
 public:
  BasicTypeNode() : basic_type(ModifierType::CUSTOM) {}
  virtual ~BasicTypeNode() = default;
  ModifierType basic_type;
  std::string name;
};

class ExpressionNode : public ASTNode {
 public:
  ExpressionNode() { type = Expression; }
  virtual ~ExpressionNode() = default;
  std::string op;
  std::unique_ptr<ExpressionNode> left;
  std::unique_ptr<ExpressionNode> right;
};

class LiteralExpressionNode : public ExpressionNode {
 public:
  LiteralExpressionNode() { type = LiteralExpression; }
  virtual ~LiteralExpressionNode() = default;
  std::string value;
};

class FormattedStringExpressionNode : public ExpressionNode {
 public:
  FormattedStringExpressionNode() { type = FormattedStringExpression; }
  std::vector<std::string> text_segments;
  std::vector<std::unique_ptr<ExpressionNode>> interpolations;
};

class IdentifierExpressionNode : public ExpressionNode {
 public:
  IdentifierExpressionNode() { type = IdentifierExpression; }
  virtual ~IdentifierExpressionNode() = default;
  std::string name;
};

class ArrayExpressionNode : public ExpressionNode {
 public:
  ArrayExpressionNode() { type = ArrayExpression; }
  std::vector<std::unique_ptr<ExpressionNode>> elements;
};

class ListComprehensionExpressionNode : public ExpressionNode {
 public:
  ListComprehensionExpressionNode() { type = ListComprehensionExpression; }
  std::unique_ptr<ExpressionNode> element;
  std::unique_ptr<IdentifierExpressionNode> iterator_variable;
  std::unique_ptr<ExpressionNode> iterable;
};

class MemberAccessExpressionNode : public ExpressionNode {
 public:
  MemberAccessExpressionNode() { type = MemberAccessExpression; }
  std::unique_ptr<ExpressionNode> object;
  std::string member_name;
};

struct StructureField {
  std::string name;
  std::unique_ptr<ExpressionNode> value;
};

class StructureExpressionNode : public ExpressionNode {
 public:
  StructureExpressionNode() { type = StructureExpression; }
  std::vector<StructureField> fields;
};

class CallExpressionNode : public ExpressionNode {
 public:
  CallExpressionNode() { type = CallExpression; }
  virtual ~CallExpressionNode() = default;
  std::string function_name;
  std::unique_ptr<ExpressionNode> callee;
  std::vector<std::unique_ptr<ExpressionNode>> arguments;
};

class StatementNode : public ASTNode {
 public:
  StatementNode() { type = Statement; }
  virtual ~StatementNode() = default;
  std::unique_ptr<ExpressionNode> expr;
  std::unique_ptr<StatementNode> body;
  std::unique_ptr<StatementNode> next;
};

class StatementListNode : public StatementNode {
 public:
  StatementListNode() { type = Statement; }
  virtual ~StatementListNode() = default;
  std::vector<std::unique_ptr<StatementNode>> statements;
};

class ForStatementNode : public StatementNode {
 public:
  ForStatementNode() { type = ForStatement; }
  virtual ~ForStatementNode() = default;
  std::unique_ptr<IdentifierExpressionNode> iterator_varible;
  std::unique_ptr<ExpressionNode> iterable;
  std::unique_ptr<StatementListNode> body;
};

class ProgramNode : public ASTNode {
 public:
  ProgramNode() { type = Program; }
  std::unique_ptr<StatementListNode> body;
};

class IfStatementNode : public StatementNode {
 public:
  IfStatementNode() { type = IfStatement; }
  std::unique_ptr<ExpressionNode> condition;
  std::unique_ptr<StatementListNode> body;
  std::vector<std::unique_ptr<IfStatementNode>> elif_branches;
  std::unique_ptr<StatementListNode> else_body;
};

class WhileStatementNode : public StatementNode {
 public:
  WhileStatementNode() { type = WhileStatement; }
  std::unique_ptr<ExpressionNode> condition;
  std::unique_ptr<StatementListNode> body;
};

class MatchStatementNode : public StatementNode {
 public:
  MatchStatementNode() { type = MatchStatement; }
  std::unique_ptr<ExpressionNode> subject;
  std::vector<std::unique_ptr<class CaseStatementNode>> cases;
};

class CaseStatementNode : public StatementNode {
 public:
  CaseStatementNode() { type = CaseStatement; }
  std::unique_ptr<ExpressionNode> pattern;
  std::unique_ptr<StatementListNode> body;
};

class BreakStatementNode : public StatementNode {
 public:
  BreakStatementNode() { type = BreakStatement; }
};

class ContinueStatementNode : public StatementNode {
 public:
  ContinueStatementNode() { type = ContinueStatement; }
};

class ReturnStatementNode : public StatementNode {
 public:
  ReturnStatementNode() { type = ReturnStatement; }
};

class FunctionDefinitionNode : public StatementNode {
 public:
  FunctionDefinitionNode() { type = FunctionDefinition; }
  virtual ~FunctionDefinitionNode() = default;
  std::string name;
  std::unordered_set<ModifierType> function_modifiers;
  std::vector<std::unique_ptr<TypeModifierNode>> parameter_types;
  std::vector<std::string> parameter_names;
  std::unique_ptr<StatementListNode> body;
};

class VariableDeclarationNode : public StatementNode {
 public:
  VariableDeclarationNode() { type = VariableDeclaration; }
  virtual ~VariableDeclarationNode() = default;
  std::string name;
  std::string declared_type_name;
  bool is_mutable = false;
  bool is_structure = false;
  std::unordered_set<ModifierType> variable_modifiers;
  std::unique_ptr<ExpressionNode> initializer;
};

class TypeDefinationNode : public StatementNode {
 public:
  TypeDefinationNode() { type = TypeDefinition; }
  virtual ~TypeDefinationNode() = default;
  std::string name;
  std::vector<std::unique_ptr<TypeModifierNode>> parameter_types;
  std::vector<std::string> parameter_names;
  struct Field {
    std::string name;
    std::unique_ptr<TypeModifierNode> declared_type;
    std::unique_ptr<ExpressionNode> default_value;
  };
  std::vector<Field> fields;
  std::unordered_set<ModifierType> type_modifiers;
  std::unique_ptr<TypeModifierNode> type_definition;
};

ParserResult parse(const std::vector<Token>& tokens, std::unique_ptr<ASTNode>& ast);

}  // namespace flux::Parser
