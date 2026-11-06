#include "parser.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void print_expression(const flux::Expression& expression, std::size_t depth);

void print_indent(std::size_t depth) {
    for (std::size_t i = 0; i < depth; ++i) {
        std::cout << "  ";
    }
}

void print_expression(const flux::Expression& expression, std::size_t depth) {
    print_indent(depth);
    switch (expression.kind) {
        case flux::ExpressionKind::Number:
            std::cout << "Number(" << expression.value << ")\n";
            break;
        case flux::ExpressionKind::String:
            std::cout << "String(" << std::quoted(expression.value) << ")\n";
            break;
        case flux::ExpressionKind::Identifier:
            std::cout << "Identifier(" << expression.value << ")\n";
            break;
        case flux::ExpressionKind::Binary:
            if (expression.value == "unary-") {
                std::cout << "Unary(-)\n";
                if (expression.right) {
                    print_expression(*expression.right, depth + 1);
                }
                break;
            }
            std::cout << "Binary(" << expression.value << ")\n";
            if (expression.left) {
                print_indent(depth + 1);
                std::cout << "Left:\n";
                print_expression(*expression.left, depth + 2);
            }
            if (expression.right) {
                print_indent(depth + 1);
                std::cout << "Right:\n";
                print_expression(*expression.right, depth + 2);
            }
            break;
        case flux::ExpressionKind::Call:
            std::cout << "Call\n";
            if (expression.left) {
                print_indent(depth + 1);
                std::cout << "Callee:\n";
                print_expression(*expression.left, depth + 2);
            }
            for (std::size_t i = 0; i < expression.arguments.size(); ++i) {
                print_indent(depth + 1);
                std::cout << "Argument " << i + 1 << ":\n";
                print_expression(*expression.arguments[i], depth + 2);
            }
            break;
        case flux::ExpressionKind::MemberAccess:
            std::cout << "MemberAccess(" << expression.value << ")\n";
            if (expression.left) {
                print_expression(*expression.left, depth + 1);
            }
            break;
    }
}

void print_statement(const flux::Statement& statement, std::size_t depth) {
    print_indent(depth);
    switch (statement.kind) {
        case flux::StatementKind::Function:
            std::cout << "Function(" << statement.name << ")\n";
            for (const auto& parameter : statement.parameters) {
                print_indent(depth + 1);
                std::cout << "Parameter(";
                if (!parameter.type.empty()) {
                    std::cout << parameter.type << " ";
                }
                std::cout << parameter.name << ")\n";
            }
            print_indent(depth + 1);
            std::cout << "Body:\n";
            for (const auto& child : statement.body) {
                print_statement(child, depth + 2);
            }
            break;
        case flux::StatementKind::PassiveFunction:
            std::cout << "PassiveFunction(" << statement.name << ")\n";
            for (const auto& parameter : statement.parameters) {
                print_indent(depth + 1);
                std::cout << "Parameter(";
                if (!parameter.type.empty()) {
                    std::cout << parameter.type << " ";
                }
                std::cout << parameter.name << ")\n";
            }
            print_indent(depth + 1);
            std::cout << "Body:\n";
            for (const auto& child : statement.body) {
                print_statement(child, depth + 2);
            }
            break;
        case flux::StatementKind::Let:
            std::cout << "Let(" << statement.name << ")\n";
            if (statement.expression) {
                print_expression(*statement.expression, depth + 1);
            }
            break;
        case flux::StatementKind::Return:
            std::cout << "Return\n";
            if (statement.expression) {
                print_expression(*statement.expression, depth + 1);
            }
            break;
        case flux::StatementKind::Print:
            std::cout << "Print\n";
            if (statement.expression) {
                print_expression(*statement.expression, depth + 1);
            }
            break;
        case flux::StatementKind::Expression:
            std::cout << "ExpressionStatement\n";
            if (statement.expression) {
                print_expression(*statement.expression, depth + 1);
            }
            break;
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <source.fl>\n";
        return 2;
    }

    std::ifstream input(argv[1]);
    if (!input) {
        std::cerr << "Error: cannot open source file: " << argv[1] << '\n';
        return 1;
    }

    const std::string source((std::istreambuf_iterator<char>(input)),
                             std::istreambuf_iterator<char>());
    try {
        const auto program = flux::parse_source(source);
        std::cout << "Program\n";
        for (const auto& statement : program) {
            print_statement(statement, 1);
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
