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

    // === Core Utility Functions ===
    bool isAtEnd() const;
    const Token& peek() const;
    const Token& previous() const;
    const Token& advance();
    bool check(TokenType type) const;
    bool match(std::initializer_list<TokenType> types);
    Token consume(TokenType type, const std::string& errorMessage);
    void synchronize();

    // === Grammar Rules for Declarations and Statements ===
    std::unique_ptr<Stmt> declaration();
    std::unique_ptr<Stmt> varDeclaration();
    std::unique_ptr<Stmt> functionDeclaration();  // Function declarations
    std::unique_ptr<Stmt> statement();
    
    // Control flow statements
    std::unique_ptr<Stmt> ifStatement();
    std::unique_ptr<Stmt> whileStatement();
    std::unique_ptr<Stmt> forStatement();
    std::unique_ptr<Stmt> blockStatement();
    std::unique_ptr<Stmt> expressionStatement();
    std::unique_ptr<Stmt> returnStatement();      // Return statements
    
    // Assignment statements
    std::unique_ptr<Stmt> assignmentStatement();
    std::unique_ptr<Stmt> parseIndexAssignmentOrExpression();  // NEW: Index assignments
    
    // === Enhanced Expression Parsing ===
    std::unique_ptr<Expr> parseRangeExpression();
    std::unique_ptr<Expr> parseArrayLiteral();               // NEW: Array literals [1, 2, 3]
    std::unique_ptr<Expr> parseIndexExpression(std::unique_ptr<Expr> object);  // NEW: Array indexing
    
    // === Enhanced Type System ===
    std::string parseTypeAnnotation();     // ENHANCED: Now supports array types
    std::string parseArrayType();          // NEW: Parse "[int]", "[[string]]", etc.
    std::vector<Parameter> parseParameterList();
    
    // === Expression Parsing with Proper Precedence ===
    std::unique_ptr<Expr> expression();
    std::unique_ptr<Expr> assignment();
    std::unique_ptr<Expr> logicalOr();
    std::unique_ptr<Expr> logicalAnd();
    std::unique_ptr<Expr> equality();
    std::unique_ptr<Expr> comparison();
    std::unique_ptr<Expr> additive();      // ENHANCED: Handles string concatenation
    std::unique_ptr<Expr> multiplicative();
    std::unique_ptr<Expr> unary();
    std::unique_ptr<Expr> call();          // ENHANCED: Handles indexing and built-ins
    std::unique_ptr<Expr> primary();       // ENHANCED: Handles array literals
    
    // === Built-in Function Helpers ===
    bool isBuiltinFunction(const std::string& name) const;
    // std::unique_ptr<Expr> parseBuiltinFunction(const std::string& name);
    
    // === Type System Helper Functions ===
    bool isTypeKeyword(TokenType type) const;
    std::string tokenTypeToTypeString(TokenType type) const;
};