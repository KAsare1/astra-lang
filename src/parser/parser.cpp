#include "parser.h"
#include <stdexcept>
#include <memory>

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens) {}

std::vector<std::unique_ptr<Stmt>> Parser::parse() {
    std::vector<std::unique_ptr<Stmt>> statements;
    while (!isAtEnd()) {
        statements.push_back(declaration());
    }
    return statements;
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::END_OF_FILE;
}

const Token& Parser::peek() const { return tokens[current]; }
const Token& Parser::previous() const { return tokens[current - 1]; }

const Token& Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

bool Parser::check(TokenType type) const {
    return !isAtEnd() && peek().type == type;
}

bool Parser::match(std::initializer_list<TokenType> types) {
    for (auto type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

std::unique_ptr<Stmt> Parser::declaration() {
    if (match({TokenType::KW_LET})) return varDeclaration();
    return statement();
}

std::unique_ptr<Stmt> Parser::varDeclaration() {
    if (!check(TokenType::IDENTIFIER))
        throw std::runtime_error("Expected variable name.");
    std::string name = advance().lexeme;

    std::unique_ptr<Expr> initializer = nullptr;
    if (match({TokenType::ASSIGN})) {
        initializer = expression();
    }
    return std::make_unique<VarDeclStmt>(name, std::move(initializer));
}

std::unique_ptr<Stmt> Parser::statement() {
    return std::make_unique<ExprStmt>(expression());
}

std::unique_ptr<Expr> Parser::expression() {
    return call();
}

std::unique_ptr<Expr> Parser::call() {
    if (!check(TokenType::IDENTIFIER)) {
        return primary();
    }
    std::string calleeName = advance().lexeme;
    if (!match({TokenType::LPAREN})) {
        return std::make_unique<VariableExpr>(calleeName);
    }
    auto callExpr = std::make_unique<CallExpr>(calleeName);
    if (!check(TokenType::RPAREN)) {
        do {
            callExpr->arguments.push_back(expression());
        } while (match({TokenType::COMMA}));
    }
    if (!match({TokenType::RPAREN})) {
        throw std::runtime_error("Expected ')' after arguments.");
    }
    return callExpr;
}

std::unique_ptr<Expr> Parser::primary() {
    if (match({TokenType::INT_LITERAL, TokenType::FLOAT_LITERAL, TokenType::STRING_LITERAL})) {
        return std::make_unique<LiteralExpr>(previous().lexeme);
    }
    if (match({TokenType::IDENTIFIER})) {
        return std::make_unique<VariableExpr>(previous().lexeme);
    }
    throw std::runtime_error("Expected expression.");
}
