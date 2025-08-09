#pragma once
#include "../lexer/lexer.h"
#include "../abstract-syntax-tree/ast.h"
#include <stdexcept>
#include <memory>
#include <vector>

class Parser {
public:
    Parser(const std::vector<Token>& tokens);
    std::vector<std::unique_ptr<Stmt>> parse();

private:
    const std::vector<Token>& tokens;
    size_t current = 0;

    // Utility functions
    bool isAtEnd() const;
    const Token& peek() const;
    const Token& previous() const;
    const Token& advance();
    bool check(TokenType type) const;
    bool match(std::initializer_list<TokenType> types);

    // Grammar rules
    std::unique_ptr<Stmt> declaration();
    std::unique_ptr<Stmt> varDeclaration();
    std::unique_ptr<Stmt> statement();
    std::unique_ptr<Expr> expression();
    std::unique_ptr<Expr> call();
    std::unique_ptr<Expr> primary();
};
