#pragma once
#include "../lexer/token.h"
#include "../abstract-syntax-tree/ast.h"
#include "../shared/symbol_table.h"
#include "../shared/error_handler.h"
#include <stdexcept>
#include <memory>
#include <vector>
#include <initializer_list>

class Parser {
public:
    Parser(const std::vector<Token>& tokens, SymbolTable& symbols, ErrorHandler& errors);
    std::vector<std::unique_ptr<Stmt>> parse();

private:
    const std::vector<Token>& tokens;
    SymbolTable& symbolTable;
    ErrorHandler& errorHandler;
    size_t current = 0;

    // Utility functions
    bool isAtEnd() const;
    const Token& peek() const;
    const Token& previous() const;
    const Token& advance();
    bool check(TokenType type) const;
    bool match(std::initializer_list<TokenType> types);
    Token consume(TokenType type, const std::string& errorMessage);
    void synchronize(); // Error recovery

    // Grammar rules for statements
    std::unique_ptr<Stmt> declaration();
    std::unique_ptr<Stmt> varDeclaration();
    std::unique_ptr<Stmt> statement();
    
    // Control flow statements
    std::unique_ptr<Stmt> ifStatement();
    std::unique_ptr<Stmt> whileStatement();
    std::unique_ptr<Stmt> forStatement();           // NEW: For loop parsing
    std::unique_ptr<Stmt> blockStatement();
    std::unique_ptr<Stmt> expressionStatement();
    
    // Assignment statement
    std::unique_ptr<Stmt> assignmentStatement();
    
    // NEW: Range expression parsing
    std::unique_ptr<Expr> parseRangeExpression();
    
    // Expression parsing with proper precedence
    std::unique_ptr<Expr> expression();
    std::unique_ptr<Expr> assignment();
    std::unique_ptr<Expr> logicalOr();
    std::unique_ptr<Expr> logicalAnd();
    std::unique_ptr<Expr> equality();
    std::unique_ptr<Expr> comparison();
    std::unique_ptr<Expr> additive();
    std::unique_ptr<Expr> multiplicative();
    std::unique_ptr<Expr> unary();
    std::unique_ptr<Expr> call();
    std::unique_ptr<Expr> primary();
};