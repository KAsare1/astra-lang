#pragma once
#include "../lexer/token.h"
#include "../abstract-syntax-tree/ast.h"
#include "../shared/symbol_table.h"
#include <stdexcept>
#include <memory>
#include <vector>
#include <initializer_list>

class Parser {
public:
    Parser(const std::vector<Token>& tokens, SymbolTable& symbols);  // Updated constructor
    std::vector<std::unique_ptr<Stmt>> parse();
    void enterScope();
    void exitScope();

private:
    const std::vector<Token>& tokens;
    SymbolTable& symbolTable;  // Reference to shared symbol table
    size_t current = 0;

    // Utility functions
    bool isAtEnd() const;
    const Token& peek() const;
    const Token& previous() const;
    const Token& advance();
    bool check(TokenType type) const;
    bool match(std::initializer_list<TokenType> types);
    Token consume(TokenType type, const std::string& errorMessage);

    // Grammar rules
    std::unique_ptr<Stmt> declaration();
    std::unique_ptr<Stmt> varDeclaration();
    std::unique_ptr<Stmt> statement();
    std::unique_ptr<Expr> expression();
    std::unique_ptr<Expr> call();
    std::unique_ptr<Expr> primary();
};