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
    
    // NEW: Control flow keywords
    {"if", TokenType::KW_IF},
    {"else", TokenType::KW_ELSE},
    {"while", TokenType::KW_WHILE},
    {"for", TokenType::KW_FOR},
    {"break", TokenType::KW_BREAK},
    {"continue", TokenType::KW_CONTINUE},
    {"true", TokenType::KW_TRUE},
    {"false", TokenType::KW_FALSE},
    {"in", TokenType::KW_IN}  // NEW: for range-based for loops

};