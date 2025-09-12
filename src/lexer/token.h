#pragma once
#include <string>

// Enhanced enum for all possible token types
enum class TokenType {
    // Keywords
    KW_LET, KW_FN, KW_STRUCT, KW_COPY, KW_UNIQUE, KW_SHARED, KW_REF, KW_MUTREF,
    KW_MATCH, KW_CASE, KW_UNSAFE, KW_EXTERN, KW_DROP, KW_RETURN, KW_DEFER, KW_SPAWN,
    
    // Control flow keywords
    KW_IF, KW_ELSE, KW_WHILE, KW_FOR, KW_BREAK, KW_CONTINUE, KW_TRUE, KW_FALSE,
    KW_IN,
    
    // Type keywords
    KW_INT, KW_DOUBLE, KW_STRING, KW_BOOL, KW_VOID,
    
    // NEW: Built-in function keywords (treated specially)
    KW_LEN, KW_PUSH, KW_POP, KW_CONCAT, KW_SUBSTR, KW_INPUT, KW_PRINTLN,
    KW_TO_STRING, KW_TO_INT, KW_TO_DOUBLE,
    
    // Identifiers and literals
    IDENTIFIER, INT_LITERAL, FLOAT_LITERAL, STRING_LITERAL, CHAR_LITERAL, NUMBER_LITERAL,
    
    // NEW: Array literal tokens
    ARRAY_START,    // Special marker for array parsing context
    ARRAY_END,      // Special marker for array parsing context
    
    // Symbols
    PLUS, MINUS, STAR, SLASH, PERCENT, ASSIGN, EQ, NEQ, LT, LTE, GT, GTE,
    AND_AND, OR_OR, AMP, AMP_MUT, BANG, DOT, COMMA, COLON, SEMICOLON,
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACK, RBRACK, ARROW, COLON_ASSIGN, FAT_ARROW, PIPE,
    
    // NEW: Enhanced operators for strings and arrays
    CONCAT_OP,      // ++ for string concatenation (alternative to +)
    
    // Special
    END_OF_FILE, UNKNOWN
};

// Token structure
struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;
    
    Token(TokenType t, const std::string &l, int ln, int col)
        : type(t), lexeme(l), line(ln), column(col) {}
};

inline const char* tokenTypeToString(TokenType type) {
    switch (type) {
        // Keywords
        case TokenType::KW_LET:      return "KW_LET";
        case TokenType::KW_FN:       return "KW_FN";
        case TokenType::KW_STRUCT:   return "KW_STRUCT";
        case TokenType::KW_COPY:     return "KW_COPY";
        case TokenType::KW_UNIQUE:   return "KW_UNIQUE";
        case TokenType::KW_SHARED:   return "KW_SHARED";
        case TokenType::KW_REF:      return "KW_REF";
        case TokenType::KW_MUTREF:   return "KW_MUTREF";
        case TokenType::KW_MATCH:    return "KW_MATCH";
        case TokenType::KW_CASE:     return "KW_CASE";
        case TokenType::KW_UNSAFE:   return "KW_UNSAFE";
        case TokenType::KW_EXTERN:   return "KW_EXTERN";
        case TokenType::KW_DROP:     return "KW_DROP";
        case TokenType::KW_RETURN:   return "KW_RETURN";
        case TokenType::KW_DEFER:    return "KW_DEFER";
        case TokenType::KW_SPAWN:    return "KW_SPAWN";
        
        // Control flow keywords
        case TokenType::KW_IF:       return "KW_IF";
        case TokenType::KW_ELSE:     return "KW_ELSE";
        case TokenType::KW_WHILE:    return "KW_WHILE";
        case TokenType::KW_FOR:      return "KW_FOR";
        case TokenType::KW_BREAK:    return "KW_BREAK";
        case TokenType::KW_CONTINUE: return "KW_CONTINUE";
        case TokenType::KW_TRUE:     return "KW_TRUE";
        case TokenType::KW_FALSE:    return "KW_FALSE";
        case TokenType::KW_IN:       return "KW_IN";
        
        // Type keywords
        case TokenType::KW_INT:      return "KW_INT";
        case TokenType::KW_DOUBLE:   return "KW_DOUBLE";
        case TokenType::KW_STRING:   return "KW_STRING";
        case TokenType::KW_BOOL:     return "KW_BOOL";
        case TokenType::KW_VOID:     return "KW_VOID";
        
        // Built-in function keywords
        case TokenType::KW_LEN:       return "KW_LEN";
        case TokenType::KW_PUSH:      return "KW_PUSH";
        case TokenType::KW_POP:       return "KW_POP";
        case TokenType::KW_CONCAT:    return "KW_CONCAT";
        case TokenType::KW_SUBSTR:    return "KW_SUBSTR";
        case TokenType::KW_INPUT:     return "KW_INPUT";
        case TokenType::KW_PRINTLN:   return "KW_PRINTLN";
        case TokenType::KW_TO_STRING: return "KW_TO_STRING";
        case TokenType::KW_TO_INT:    return "KW_TO_INT";
        case TokenType::KW_TO_DOUBLE: return "KW_TO_DOUBLE";

        // Identifiers and literals
        case TokenType::IDENTIFIER:      return "IDENTIFIER";
        case TokenType::INT_LITERAL:     return "INT_LITERAL";
        case TokenType::FLOAT_LITERAL:   return "FLOAT_LITERAL";
        case TokenType::STRING_LITERAL:  return "STRING_LITERAL";
        case TokenType::CHAR_LITERAL:    return "CHAR_LITERAL";
        case TokenType::NUMBER_LITERAL:  return "NUMBER_LITERAL";

        // Array tokens
        case TokenType::ARRAY_START:     return "ARRAY_START";
        case TokenType::ARRAY_END:       return "ARRAY_END";

        // Symbols
        case TokenType::PLUS:        return "PLUS";
        case TokenType::MINUS:       return "MINUS";
        case TokenType::STAR:        return "STAR";
        case TokenType::SLASH:       return "SLASH";
        case TokenType::PERCENT:     return "PERCENT";
        case TokenType::ASSIGN:      return "ASSIGN";
        case TokenType::EQ:          return "EQ";
        case TokenType::NEQ:         return "NEQ";
        case TokenType::LT:          return "LT";
        case TokenType::LTE:         return "LTE";
        case TokenType::GT:          return "GT";
        case TokenType::GTE:         return "GTE";
        case TokenType::AND_AND:     return "AND_AND";
        case TokenType::OR_OR:       return "OR_OR";
        case TokenType::AMP:         return "AMP";
        case TokenType::AMP_MUT:     return "AMP_MUT";
        case TokenType::BANG:        return "BANG";
        case TokenType::DOT:         return "DOT";
        case TokenType::COMMA:       return "COMMA";
        case TokenType::COLON:       return "COLON";
        case TokenType::FAT_ARROW:   return "FAT_ARROW";
        case TokenType::SEMICOLON:   return "SEMICOLON";
        case TokenType::LPAREN:      return "LPAREN";
        case TokenType::RPAREN:      return "RPAREN";
        case TokenType::LBRACE:      return "LBRACE";
        case TokenType::RBRACE:      return "RBRACE";
        case TokenType::LBRACK:      return "LBRACK";
        case TokenType::RBRACK:      return "RBRACK";
        case TokenType::ARROW:       return "ARROW";
        case TokenType::COLON_ASSIGN: return "COLON_ASSIGN";
        case TokenType::PIPE:        return "PIPE";
        case TokenType::CONCAT_OP:   return "CONCAT_OP";
        case TokenType::UNKNOWN:     return "UNKNOWN";

        // Special
        case TokenType::END_OF_FILE: return "EOF";

        default: return "UNKNOWN";
    }
}