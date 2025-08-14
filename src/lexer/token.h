#pragma once
#include <string>



// Enum for all possible token types
enum class TokenType {
    // Keywords
    KW_LET, KW_FN, KW_STRUCT, KW_COPY, KW_UNIQUE, KW_SHARED, KW_REF, KW_MUTREF,
    KW_MATCH, KW_CASE, KW_UNSAFE, KW_EXTERN, KW_DROP, KW_RETURN, KW_DEFER, KW_SPAWN,
    // Identifiers and literals
    IDENTIFIER, INT_LITERAL, FLOAT_LITERAL, STRING_LITERAL, CHAR_LITERAL, NUMBER_LITERAL,
    // Symbols
    PLUS, MINUS, STAR, SLASH, PERCENT, ASSIGN, EQ, NEQ, LT, LTE, GT, GTE,
    AND_AND, OR_OR, AMP, AMP_MUT, BANG, DOT, COMMA, COLON, SEMICOLON,
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACK, RBRACK, ARROW, EQUAL, COLON_ASSIGN, FAT_ARROW, PIPE, 

    // Special
    END_OF_FILE, UNKNOWN
};


// Token structure
struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;
    
    // Constructor
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

        // Identifiers and literals
        case TokenType::IDENTIFIER:      return "IDENTIFIER";
        case TokenType::INT_LITERAL:     return "INT_LITERAL";
        case TokenType::FLOAT_LITERAL:   return "FLOAT_LITERAL";
        case TokenType::STRING_LITERAL:  return "STRING_LITERAL";
        case TokenType::CHAR_LITERAL:    return "CHAR_LITERAL";
        case TokenType::NUMBER_LITERAL:  return "NUMBER_LITERAL";

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
        case TokenType::EQUAL:       return "EQUAL";
        case TokenType::COLON_ASSIGN: return "COLON_ASSIGN";
        case TokenType::PIPE: return "PIPE";
        case TokenType::UNKNOWN: return "UNKNOWN";

        // Special
        case TokenType::END_OF_FILE: return "EOF";

        default: return "UNKNOWN";
    }
}

