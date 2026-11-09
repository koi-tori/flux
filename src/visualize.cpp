#include "parser.hpp"

#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace {

using namespace flux::Parser;

void printNode(const ASTNode* node, std::ostream& output, std::size_t depth,
               const std::string& label);

void printLine(std::ostream& output, std::size_t depth,
               const std::string& label) {
  output << std::string(depth * 2, ' ') << label << '\n';
}

std::string modifierName(ModifierType modifier) {
  switch (modifier) {
    case ModifierType::PASSIVE: return "passive";
    case ModifierType::ACTIVE: return "active";
    case ModifierType::IMMEDIATE: return "immediate";
    case ModifierType::MUTABLE: return "mutable";
    case ModifierType::IMMUTABLE: return "immutable";
    case ModifierType::CHAR: return "char";
    case ModifierType::SHORT: return "short";
    case ModifierType::INT: return "int";
    case ModifierType::LONGLONG: return "long";
    case ModifierType::FLOAT: return "float";
    case ModifierType::DOUBLE: return "double";
    case ModifierType::BOOL: return "bool";
    case ModifierType::STRING: return "string";
    case ModifierType::CUSTOM: return "custom";
  }
  return "unknown";
}

template <typename Modifiers>
std::string formatModifiers(const Modifiers& modifiers) {
  std::ostringstream result;
  bool first = true;
  for (const ModifierType modifier : modifiers) {
    if (!first) result << ", ";
    result << modifierName(modifier);
    first = false;
  }
  return result.str();
}

void printStatementList(const StatementListNode* statements,
                        std::ostream& output, std::size_t depth,
                        const std::string& label) {
  if (statements == nullptr) return;
  printLine(output, depth, label);
  for (std::size_t i = 0; i < statements->statements.size(); ++i) {
    printNode(statements->statements[i].get(), output, depth + 1,
              "Statement " + std::to_string(i + 1));
  }
}

void printNode(const ASTNode* node, std::ostream& output, std::size_t depth,
               const std::string& label) {
  if (node == nullptr) return;

  if (const auto* program = dynamic_cast<const ProgramNode*>(node)) {
    printLine(output, depth, label.empty() ? "Program" : label);
    printStatementList(program->body.get(), output, depth + 1, "Statements");
    return;
  }

  if (const auto* statements = dynamic_cast<const StatementListNode*>(node)) {
    printStatementList(statements, output, depth, label);
    return;
  }

  if (const auto* function = dynamic_cast<const FunctionDefinitionNode*>(node)) {
    printLine(output, depth, label + ": FunctionDefinition " + function->name);
    if (!function->function_modifiers.empty()) {
      printLine(output, depth + 1,
                "Modifiers: " + formatModifiers(function->function_modifiers));
    }
    for (std::size_t i = 0; i < function->parameter_names.size(); ++i) {
      printLine(output, depth + 1,
                "Parameter: " + function->parameter_names[i]);
      if (i < function->parameter_types.size()) {
        printNode(function->parameter_types[i].get(), output, depth + 2, "Type");
      }
    }
    printStatementList(function->body.get(), output, depth + 1, "Body");
    return;
  }

  if (const auto* declaration =
          dynamic_cast<const VariableDeclarationNode*>(node)) {
    printLine(output, depth,
              label + ": " +
                  std::string(declaration->is_mutable ? "VarDeclaration "
                                                      : "LetDeclaration ") +
                  declaration->name);
    if (!declaration->variable_modifiers.empty()) {
      printLine(output, depth + 1,
                "Modifiers: " + formatModifiers(declaration->variable_modifiers));
    }
    printNode(declaration->initializer.get(), output, depth + 1, "Initializer");
    return;
  }

  if (const auto* branch = dynamic_cast<const IfStatementNode*>(node)) {
    printLine(output, depth, label + ": IfStatement");
    printNode(branch->condition.get(), output, depth + 1, "Condition");
    printStatementList(branch->body.get(), output, depth + 1, "Body");
    for (const auto& elifBranch : branch->elif_branches) {
      printNode(elifBranch.get(), output, depth + 1, "Elif");
    }
    printStatementList(branch->else_body.get(), output, depth + 1, "Else");
    return;
  }

  if (const auto* loop = dynamic_cast<const WhileStatementNode*>(node)) {
    printLine(output, depth, label + ": WhileStatement");
    printNode(loop->condition.get(), output, depth + 1, "Condition");
    printStatementList(loop->body.get(), output, depth + 1, "Body");
    return;
  }

  if (const auto* loop = dynamic_cast<const ForStatementNode*>(node)) {
    printLine(output, depth, label + ": ForStatement");
    printNode(loop->element.get(), output, depth + 1, "Element");
    printNode(loop->iterator.get(), output, depth + 1, "Iterator");
    printStatementList(loop->body.get(), output, depth + 1, "Body");
    return;
  }

  if (const auto* match = dynamic_cast<const MatchStatementNode*>(node)) {
    printLine(output, depth, label + ": MatchStatement");
    printNode(match->subject.get(), output, depth + 1, "Subject");
    for (std::size_t i = 0; i < match->cases.size(); ++i) {
      printNode(match->cases[i].get(), output, depth + 1,
                "Case " + std::to_string(i + 1));
    }
    return;
  }

  if (const auto* caseNode = dynamic_cast<const CaseStatementNode*>(node)) {
    printLine(output, depth, label + ": CaseStatement");
    printNode(caseNode->pattern.get(), output, depth + 1, "Pattern");
    printStatementList(caseNode->body.get(), output, depth + 1, "Body");
    return;
  }

  if (const auto* type = dynamic_cast<const BasicTypeNode*>(node)) {
    printLine(output, depth, label + ": " +
                                (type->name.empty() ? modifierName(type->basic_type)
                                                    : type->name));
    for (const auto& subType : type->sub_types) {
      printNode(subType.get(), output, depth + 1, "Modifier");
    }
    return;
  }

  if (const auto* call = dynamic_cast<const CallExpressionNode*>(node)) {
    printLine(output, depth, label + ": Call " + call->function_name);
    for (std::size_t i = 0; i < call->arguments.size(); ++i) {
      printNode(call->arguments[i].get(), output, depth + 1,
                "Argument " + std::to_string(i + 1));
    }
    return;
  }

  if (const auto* literal = dynamic_cast<const LiteralExpressionNode*>(node)) {
    printLine(output, depth, label + ": Literal " + literal->value);
    return;
  }

  if (const auto* identifier =
          dynamic_cast<const IdentifierExpressionNode*>(node)) {
    printLine(output, depth, label + ": Identifier " + identifier->name);
    return;
  }

  if (const auto* expression = dynamic_cast<const ExpressionNode*>(node)) {
    if (node->type == UnaryExpression) {
      printLine(output, depth, label + ": UnaryExpression " + expression->op);
      printNode(expression->right.get(), output, depth + 1, "Operand");
    } else if (node->type == BinaryExpression) {
      printLine(output, depth, label + ": BinaryExpression " + expression->op);
      printNode(expression->left.get(), output, depth + 1, "Left");
      printNode(expression->right.get(), output, depth + 1, "Right");
    } else {
      printLine(output, depth, label + ": Expression");
    }
    return;
  }

  if (const auto* returned = dynamic_cast<const ReturnStatementNode*>(node)) {
    printLine(output, depth, label + ": ReturnStatement");
    printNode(returned->expr.get(), output, depth + 1, "Value");
    return;
  }

  if (node->type == BreakStatement) {
    printLine(output, depth, label + ": BreakStatement");
    return;
  }
  if (node->type == ContinueStatement) {
    printLine(output, depth, label + ": ContinueStatement");
    return;
  }

  if (const auto* statement = dynamic_cast<const StatementNode*>(node)) {
    const std::string kind =
        node->type == PrintStatement ? "PrintStatement" : "Statement";
    printLine(output, depth, label + ": " + kind);
    printNode(statement->expr.get(), output, depth + 1, "Expression");
    printNode(statement->body.get(), output, depth + 1, "Body");
    printNode(statement->next.get(), output, depth + 1, "Next");
    return;
  }

  printLine(output, depth, label + ": ASTNode");
}

void printSyntaxTree(const ASTNode* root, std::ostream& output) {
  printNode(root, output, 0, "Program");
}

}  // namespace

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "usage: flux-parser <source-file>\n";
    return 2;
  }

  std::ifstream input(argv[1]);
  if (!input) {
    std::cerr << "unable to open source file: " << argv[1] << '\n';
    return 2;
  }

  const std::string source((std::istreambuf_iterator<char>(input)),
                           std::istreambuf_iterator<char>());
  std::vector<flux::Parser::Token> tokens;
  flux::Parser::ParserResult result =
      flux::Parser::tokenize(source, tokens);
  if (!result.success) {
    std::cerr << result.errorMessage << '\n';
    return 1;
  }

  std::unique_ptr<flux::Parser::ASTNode> ast;
  result = flux::Parser::parse(tokens, ast);
  if (!result.success) {
    std::cerr << result.errorMessage << '\n';
    return 1;
  }
  printSyntaxTree(ast.get(), std::cout);
  return 0;
}