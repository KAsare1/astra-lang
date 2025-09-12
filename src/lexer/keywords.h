#pragma once
#include <unordered_map>
#include <string>
#include "token.h"

static const std::unordered_map<std::string, TokenType> keywords = {
    {"let", TokenType::KW_LET},
    {"fn", TokenType::KW_FN},
    {"struct", TokenType::KW_STRUCT},
    {"copy", TokenType::KW_COPY},
    {"unique", TokenType::KW_UNIQUE},
    {"shared", TokenType::KW_SHARED},
    {"ref", TokenType::KW_REF},
    {"mutref", TokenType::KW_MUTREF},
    {"match", TokenType::KW_MATCH},
    {"case", TokenType::KW_CASE},
    {"unsafe", TokenType::KW_UNSAFE},
    {"extern", TokenType::KW_EXTERN},
    {"drop", TokenType::KW_DROP},
    {"return", TokenType::KW_RETURN},
    {"defer", TokenType::KW_DEFER},
    {"spawn", TokenType::KW_SPAWN},
    
    // Control flow keywords
    {"if", TokenType::KW_IF},
    {"else", TokenType::KW_ELSE},
    {"while", TokenType::KW_WHILE},
    {"for", TokenType::KW_FOR},
    {"break", TokenType::KW_BREAK},
    {"continue", TokenType::KW_CONTINUE},
    {"true", TokenType::KW_TRUE},
    {"false", TokenType::KW_FALSE},
    {"in", TokenType::KW_IN},
    
    // Type keywords for function parameters and returns
    {"int", TokenType::KW_INT},
    {"double", TokenType::KW_DOUBLE},
    {"string", TokenType::KW_STRING},
    {"bool", TokenType::KW_BOOL},
    {"void", TokenType::KW_VOID},
    
    // NEW: Built-in function keywords
    // Core data structure functions
    {"len", TokenType::KW_LEN},           // Generic length function
    {"push", TokenType::KW_PUSH},         // Array append
    {"pop", TokenType::KW_POP},           // Array pop
    
    // String functions
    {"concat", TokenType::KW_CONCAT},     // String concatenation
    {"substr", TokenType::KW_SUBSTR},     // Substring extraction
    
    // I/O functions
    {"input", TokenType::KW_INPUT},       // Read user input
    {"println", TokenType::KW_PRINTLN},   // Print with newline
    
    // Type conversion functions
    {"to_string", TokenType::KW_TO_STRING}, // Convert to string
    {"to_int", TokenType::KW_TO_INT},       // Parse integer
    {"to_double", TokenType::KW_TO_DOUBLE}  // Parse double
};