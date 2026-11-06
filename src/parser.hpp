#pragma once

#include <memory>
#include <string>
#include <vector>

namespace flux {

enum class ExpressionKind {
    Number,
    String,
    Identifier,
    Binary,
    Call
};

struct Expression {
    ExpressionKind kind;
    std::string value;
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
    std::vector<std::unique_ptr<Expression>> arguments;
};

struct Parameter {
    std::string type;
    std::string name;
};

enum class StatementKind {
    Function,
    Let,
    Return,
    Print,
    Expression
};

struct Statement {
    StatementKind kind;
    std::string name;
    std::vector<Parameter> parameters;
    std::vector<Statement> body;
    std::unique_ptr<Expression> expression;
};

std::vector<Statement> parse_source(std::string source);

}  // namespace flux
