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
    Call,
    MemberAccess
};

struct Expression {
    ExpressionKind kind;
    std::string value;
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
    std::vector<std::unique_ptr<Expression>> arguments;
};

enum class TypeKind {
    Basic,
    Unspecified
};

struct Type {
    TypeKind kind;
    std::string name;
};

struct Parameter {
    std::string name;
    Type type;
};

enum class StatementKind {
    Function,
    PassiveFunction,
    ActiveFunction,
    Let,
    Assignment,
    Delete,
    Return,
    Print,
    Accept,
    YieldPause,
    YieldExtract,
    Defer,
    Expression
};

struct Statement {
    StatementKind kind;
    std::string name;
    std::vector<Parameter> parameters;
    std::vector<Statement> body;
    std::unique_ptr<Expression> target;
    std::unique_ptr<Expression> expression;
};

std::vector<Statement> parse_source(std::string source);

}  // namespace flux
