#include "parser.hpp"

#include <cctype>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace flux {

class Parser {
public:
    explicit Parser(std::string source) : source_(std::move(source)) {
        tokenize();
    }

    std::vector<Statement> parse() {
        std::vector<Statement> statements;
        skip_newlines();
        while (!check(TokenKind::End)) {
            statements.push_back(parse_function());
            skip_newlines();
        }
        return statements;
    }

private:
    enum class TokenKind {
        Identifier,
        Number,
        String,
        Dot,
        Plus,
        Minus,
        Star,
        Slash,
        Equal,
        Comma,
        Colon,
        LeftParen,
        RightParen,
        Newline,
        Indent,
        Dedent,
        End
    };

    struct Token {
        TokenKind kind;
        std::string text;
        std::size_t line;
        std::size_t column;
    };

    std::string source_;
    std::vector<Token> tokens_;
    std::size_t current_ = 0;
    std::vector<std::size_t> indentation_{0};
    std::size_t offset_ = 0;
    std::size_t line_ = 1;
    std::size_t column_ = 1;

    [[noreturn]] void error(const Token& token, const std::string& message) const {
        throw std::runtime_error(
            "Parse error at " + std::to_string(token.line) + ":" +
            std::to_string(token.column) + ": " + message);
    }

    char peek(std::size_t lookahead = 0) const {
        const auto position = offset_ + lookahead;
        return position < source_.size() ? source_[position] : '\0';
    }

    char advance() {
        const char character = source_[offset_++];
        if (character == '\n') {
            ++line_;
            column_ = 1;
        } else {
            ++column_;
        }
        return character;
    }

    void add_token(TokenKind kind, std::string text, std::size_t line,
                   std::size_t column) {
        tokens_.push_back({kind, std::move(text), line, column});
    }

    void tokenize() {
        bool at_line_start = true;
        while (offset_ < source_.size()) {
            if (at_line_start) {
                const auto start_line = line_;
                std::size_t indent = 0;
                while (peek() == ' ' || peek() == '\t') {
                    indent += advance() == '\t' ? 4 : 1;
                }

                if (peek() == '\n') {
                    advance();
                    continue;
                }
                if (peek() == '#') {
                    while (peek() != '\0' && peek() != '\n') {
                        advance();
                    }
                    if (peek() == '\n') {
                        advance();
                    }
                    continue;
                }

                if (indent > indentation_.back()) {
                    indentation_.push_back(indent);
                    add_token(TokenKind::Indent, "", start_line, 1);
                } else if (indent < indentation_.back()) {
                    while (indent < indentation_.back()) {
                        indentation_.pop_back();
                        add_token(TokenKind::Dedent, "", start_line, 1);
                    }
                    if (indent != indentation_.back()) {
                        throw std::runtime_error(
                            "Parse error at " + std::to_string(start_line) +
                            ":1: inconsistent indentation");
                    }
                }
                at_line_start = false;
                continue;
            }

            const char character = peek();
            if (character == ' ' || character == '\t' || character == '\r') {
                advance();
                continue;
            }
            if (character == '\n') {
                const auto token_line = line_;
                const auto token_column = column_;
                advance();
                add_token(TokenKind::Newline, "", token_line, token_column);
                at_line_start = true;
                continue;
            }
            if (character == '#') {
                while (peek() != '\0' && peek() != '\n') {
                    advance();
                }
                continue;
            }

            const auto token_line = line_;
            const auto token_column = column_;
            if (std::isalpha(static_cast<unsigned char>(character)) ||
                character == '_') {
                std::string text;
                while (std::isalnum(static_cast<unsigned char>(peek())) ||
                       peek() == '_') {
                    text += advance();
                }
                add_token(TokenKind::Identifier, std::move(text), token_line,
                          token_column);
                continue;
            }
            if (std::isdigit(static_cast<unsigned char>(character))) {
                std::string text;
                while (std::isdigit(static_cast<unsigned char>(peek()))) {
                    text += advance();
                }
                add_token(TokenKind::Number, std::move(text), token_line,
                          token_column);
                continue;
            }
            if (character == '"' || character == '\'') {
                const char quote = advance();
                std::string text;
                while (peek() != quote && peek() != '\0' && peek() != '\n') {
                    char value = advance();
                    if (value == '\\') {
                        const char escaped = advance();
                        switch (escaped) {
                            case 'n': value = '\n'; break;
                            case 't': value = '\t'; break;
                            case '\\': value = '\\'; break;
                            case '"': value = '"'; break;
                            case '\'': value = '\''; break;
                            default:
                                throw std::runtime_error(
                                    "Parse error at " + std::to_string(line_) +
                                    ":" + std::to_string(column_) +
                                    ": invalid string escape");
                        }
                    }
                    text += value;
                }
                if (peek() != quote) {
                    throw std::runtime_error(
                        "Parse error at " + std::to_string(token_line) + ":" +
                        std::to_string(token_column) + ": unterminated string");
                }
                advance();
                add_token(TokenKind::String, std::move(text), token_line,
                          token_column);
                continue;
            }

            advance();
            switch (character) {
                case '.': add_token(TokenKind::Dot, ".", token_line, token_column); break;
                case '+': add_token(TokenKind::Plus, "+", token_line, token_column); break;
                case '-': add_token(TokenKind::Minus, "-", token_line, token_column); break;
                case '*': add_token(TokenKind::Star, "*", token_line, token_column); break;
                case '/': add_token(TokenKind::Slash, "/", token_line, token_column); break;
                case '=': add_token(TokenKind::Equal, "=", token_line, token_column); break;
                case ',': add_token(TokenKind::Comma, ",", token_line, token_column); break;
                case ':': add_token(TokenKind::Colon, ":", token_line, token_column); break;
                case '(': add_token(TokenKind::LeftParen, "(", token_line, token_column); break;
                case ')': add_token(TokenKind::RightParen, ")", token_line, token_column); break;
                default:
                    throw std::runtime_error(
                        "Parse error at " + std::to_string(token_line) + ":" +
                        std::to_string(token_column) + ": unexpected character");
            }
        }

        while (indentation_.size() > 1) {
            indentation_.pop_back();
            add_token(TokenKind::Dedent, "", line_, column_);
        }
        add_token(TokenKind::End, "", line_, column_);
    }

    const Token& peek_token() const {
        return tokens_[current_];
    }

    bool check(TokenKind kind) const {
        return peek_token().kind == kind;
    }

    bool match(TokenKind kind) {
        if (!check(kind)) {
            return false;
        }
        ++current_;
        return true;
    }

    Token consume(TokenKind kind, const std::string& message) {
        if (check(kind)) {
            return tokens_[current_++];
        }
        error(peek_token(), message);
    }

    void skip_newlines() {
        while (match(TokenKind::Newline)) {
        }
    }

    Statement parse_function() {
        const Token keyword = consume(TokenKind::Identifier, "expected 'def'");
        if (keyword.text != "def") {
            error(keyword, "expected a function definition");
        }

        const bool is_passive = check(TokenKind::Identifier) &&
                                peek_token().text == "passive";
        if (is_passive) {
            ++current_;
        }

        Statement function{};
        function.kind = is_passive ? StatementKind::PassiveFunction
                                   : StatementKind::Function;
        function.name = consume(TokenKind::Identifier, "expected function name").text;
        consume(TokenKind::LeftParen, "expected '(' after function name");
        if (!check(TokenKind::RightParen)) {
            do {
                const auto first = consume(TokenKind::Identifier, "expected parameter");
                if (check(TokenKind::Identifier)) {
                    function.parameters.push_back(
                        {first.text, consume(TokenKind::Identifier, "expected parameter name").text});
                } else {
                    function.parameters.push_back({"", first.text});
                }
            } while (match(TokenKind::Comma));
        }
        consume(TokenKind::RightParen, "expected ')' after parameters");
        consume(TokenKind::Colon, "expected ':' after function signature");
        consume(TokenKind::Newline, "expected a newline after ':'");
        consume(TokenKind::Indent, "expected an indented function body");

        skip_newlines();
        while (!check(TokenKind::Dedent) && !check(TokenKind::End)) {
            function.body.push_back(parse_statement());
            skip_newlines();
        }
        consume(TokenKind::Dedent, "expected the end of the function body");
        return function;
    }

    Statement parse_statement() {
        if (check(TokenKind::Identifier)) {
            if (peek_token().text == "def") {
                return parse_function();
            }
            if (peek_token().text == "let") {
                ++current_;
                Statement statement{};
                statement.kind = StatementKind::Let;
                statement.name = consume(TokenKind::Identifier, "expected variable name").text;
                consume(TokenKind::Equal, "expected '=' after variable name");
                statement.expression = parse_expression();
                consume(TokenKind::Newline, "expected a newline after statement");
                return statement;
            }
            if (peek_token().text == "return") {
                ++current_;
                Statement statement{};
                statement.kind = StatementKind::Return;
                statement.expression = parse_expression();
                consume(TokenKind::Newline, "expected a newline after statement");
                return statement;
            }
            if (peek_token().text == "print") {
                ++current_;
                Statement statement{};
                statement.kind = StatementKind::Print;
                if (match(TokenKind::LeftParen)) {
                    statement.expression = parse_expression();
                    consume(TokenKind::RightParen, "expected ')' after print expression");
                } else {
                    statement.expression = parse_expression();
                }
                consume(TokenKind::Newline, "expected a newline after statement");
                return statement;
            }
        }

        Statement statement{};
        statement.kind = StatementKind::Expression;
        statement.expression = parse_expression();
        consume(TokenKind::Newline, "expected a newline after statement");
        return statement;
    }

    std::unique_ptr<Expression> parse_expression() {
        return parse_additive();
    }

    std::unique_ptr<Expression> parse_additive() {
        auto expression = parse_multiplicative();
        while (check(TokenKind::Plus) || check(TokenKind::Minus)) {
            const Token operation = tokens_[current_++];
            auto binary = std::make_unique<Expression>();
            binary->kind = ExpressionKind::Binary;
            binary->value = operation.text;
            binary->left = std::move(expression);
            binary->right = parse_multiplicative();
            expression = std::move(binary);
        }
        return expression;
    }

    std::unique_ptr<Expression> parse_multiplicative() {
        auto expression = parse_unary();
        while (check(TokenKind::Star) || check(TokenKind::Slash)) {
            const Token operation = tokens_[current_++];
            auto binary = std::make_unique<Expression>();
            binary->kind = ExpressionKind::Binary;
            binary->value = operation.text;
            binary->left = std::move(expression);
            binary->right = parse_unary();
            expression = std::move(binary);
        }
        return expression;
    }

    std::unique_ptr<Expression> parse_unary() {
        if (match(TokenKind::Minus)) {
            auto unary = std::make_unique<Expression>();
            unary->kind = ExpressionKind::Binary;
            unary->value = "unary-";
            unary->right = parse_unary();
            return unary;
        }
        return parse_postfix();
    }

    std::unique_ptr<Expression> parse_postfix() {
        auto expression = parse_primary();
        while (true) {
            if (match(TokenKind::Dot)) {
                auto member = std::make_unique<Expression>();
                member->kind = ExpressionKind::MemberAccess;
                member->left = std::move(expression);
                member->value =
                    consume(TokenKind::Identifier, "expected member name after '.'").text;
                expression = std::move(member);
                continue;
            }

            if (!match(TokenKind::LeftParen)) {
                break;
            }

            auto call = std::make_unique<Expression>();
            call->kind = ExpressionKind::Call;
            call->left = std::move(expression);
            if (!check(TokenKind::RightParen)) {
                do {
                    call->arguments.push_back(parse_expression());
                } while (match(TokenKind::Comma));
            }
            consume(TokenKind::RightParen, "expected ')' after arguments");
            expression = std::move(call);
        }
        return expression;
    }

    std::unique_ptr<Expression> parse_primary() {
        const Token token = peek_token();
        if (match(TokenKind::Number) || match(TokenKind::String)) {
            auto expression = std::make_unique<Expression>();
            expression->kind = token.kind == TokenKind::Number
                                   ? ExpressionKind::Number
                                   : ExpressionKind::String;
            expression->value = token.text;
            return expression;
        }
        if (match(TokenKind::Identifier)) {
            auto expression = std::make_unique<Expression>();
            expression->kind = ExpressionKind::Identifier;
            expression->value = token.text;
            return expression;
        }
        if (match(TokenKind::LeftParen)) {
            auto expression = parse_expression();
            consume(TokenKind::RightParen, "expected ')' after expression");
            return expression;
        }
        error(token, "expected an expression");
    }
};

std::vector<Statement> parse_source(std::string source) {
    return Parser(std::move(source)).parse();
}

}  // namespace flux