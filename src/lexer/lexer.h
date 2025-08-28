#pragma once
#include <string>
#include <vector>
#include "token.h"
#include "../shared/symbol_table.h"
#include "../shared/error_handler.h"

class Lexer {
public:
    Lexer(const std::string &src, SymbolTable& symbols, ErrorHandler& errors);
    std::vector<Token> tokenize();
    void enterScope(); 
    void exitScope(); 

private:
    const std::string &source;
    SymbolTable& symbolTable; 
    ErrorHandler& errorHandler;
    size_t pos = 0;
    int line = 1;
    int column = 1;
    std::vector<Token> tokens;

    char peek() const;
    char peekNext() const;
    char advance();
    bool match(char expected);
    bool isAtEnd() const;
    void addToken(TokenType type, const std::string &lexeme);
    TokenType keywordType(const std::string &word) const;
    void skipWhitespace();
    void skipComment();
    void identifier();
    void number();
    void stringLiteral();
    void charLiteral();
    void symbol();
};