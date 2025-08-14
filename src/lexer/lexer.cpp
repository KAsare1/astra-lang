#include "lexer.h"
#include <cctype>
#include <stdexcept>
#include "keywords.h"


// Constructor: initializes the lexer with the source code string
Lexer::Lexer(const std::string &src) : source(src) {}


// Returns the current character without consuming it
char Lexer::peek() const {
    return isAtEnd() ? '\0' : source[pos];
}


// Returns the next character without consuming it
char Lexer::peekNext() const {
    return (pos + 1 >= source.size()) ? '\0' : source[pos + 1];
}


// Consumes and returns the current character, advancing the position
char Lexer::advance() {
    char c = source[pos++];
    column++;
    return c;
}


// If the current character matches 'expected', consumes it and returns true
bool Lexer::match(char expected) {
    if (isAtEnd() || source[pos] != expected) return false;
    pos++;
    column++;
    return true;
}


// Returns true if we've reached the end of the source
bool Lexer::isAtEnd() const {
    return pos >= source.size();
}


// Adds a token of the given type and lexeme to the token list
void Lexer::addToken(TokenType type, const std::string &lexeme) {
    tokens.push_back({type, lexeme, line, column - (int)lexeme.size()});
}


// Returns the TokenType for a given word, or IDENTIFIER if not a keyword
TokenType Lexer::keywordType(const std::string &word) const {
    auto it = keywords.find(word);
    if (it != keywords.end()) return it->second;
    return TokenType::IDENTIFIER;
}


// Skips whitespace characters (space, tab, newline, etc.)
void Lexer::skipWhitespace() {
    for (;;) {
        char c = peek();
        switch (c) {
            case ' ': case '\r': case '\t':
                advance();
                break;
            case '\n':
                advance();
                line++;
                column = 1;
                break;
            default:
                return;
        }
    }
}


// Skips single-line (//) and multi-line (/* ... */) comments
void Lexer::skipComment() {
    if (match('/')) {
        while (peek() != '\n' && !isAtEnd()) advance();
    } else if (match('*')) {
        while (!(peek() == '*' && peekNext() == '/') && !isAtEnd()) {
            if (peek() == '\n') { line++; column = 1; }
            advance();
        }
        if (!isAtEnd()) { advance(); advance(); }
    }
}


// Lexes an identifier or keyword
void Lexer::identifier() {
    size_t start = pos - 1;
    while (std::isalnum(peek()) || peek() == '_') advance();
    std::string word = source.substr(start, pos - start);

    TokenType type = keywordType(word);
    if (type == TokenType::IDENTIFIER) {
        // Add the identifier to the symbol table
        if (!symbolTable.isDeclared(word)) {
            symbolTable.declare(word);
        }
    }

    addToken(type, word);
}


// Lexes a number literal (integer or float)
void Lexer::number() {
    size_t start = pos - 1;
    bool isFloat = false;
    while (std::isdigit(peek())) advance();
    if (peek() == '.' && std::isdigit(peekNext())) {
        isFloat = true;
        advance();
        while (std::isdigit(peek())) advance();
    }
    std::string num = source.substr(start, pos - start);
    addToken(isFloat ? TokenType::FLOAT_LITERAL : TokenType::INT_LITERAL, num);
}


// Lexes a string literal
void Lexer::stringLiteral() {
    size_t startPos = pos;
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') { line++; column = 1; }
        advance();
    }
    if (isAtEnd()) throw std::runtime_error("Unterminated string");
    advance();
    std::string val = source.substr(startPos, pos - startPos - 1);
    addToken(TokenType::STRING_LITERAL, val);
}


// Lexes a character literal
void Lexer::charLiteral() {
    char c = advance();
    if (c == '\\') advance();
    if (peek() != '\'') throw std::runtime_error("Unterminated char literal");
    advance();
    addToken(TokenType::CHAR_LITERAL, std::string(1, c));
}


// Lexes a symbol or operator token
void Lexer::symbol() {
    char c = advance();
    switch (c) {
        case '+': addToken(TokenType::PLUS, "+"); break;
        case '-':
            if (match('>')) addToken(TokenType::ARROW, "->");
            else addToken(TokenType::MINUS, "-");
            break;
        case '*': addToken(TokenType::STAR, "*"); break;
        case '/':
            if (peek() == '/' || peek() == '*') skipComment();
            else addToken(TokenType::SLASH, "/");
            break;
        case '%': addToken(TokenType::PERCENT, "%"); break;
        case '=':
            if (match('>')) addToken(TokenType::FAT_ARROW, "=>");
            else if (match('=')) addToken(TokenType::EQ, "==");
            else addToken(TokenType::ASSIGN, "=");
            break;

        case '!':
            if (match('=')) addToken(TokenType::NEQ, "!=");
            else addToken(TokenType::BANG, "!");
            break;
        case '<':
            if (match('=')) addToken(TokenType::LTE, "<=");
            else addToken(TokenType::LT, "<");
            break;
        case '>':
            if (match('=')) addToken(TokenType::GTE, ">=");
            else addToken(TokenType::GT, ">");
            break;
        case '&':
            if (match('&')) addToken(TokenType::AND_AND, "&&");
            else if (match('m')) { // rudimentary check for &mut
                if (match('u') && match('t')) addToken(TokenType::AMP_MUT, "&mut");
            } else addToken(TokenType::AMP, "&");
            break;
        case '|':
            if (match('|')) addToken(TokenType::OR_OR, "||");
            break;
        case '.': addToken(TokenType::DOT, "."); break;
        case ',': addToken(TokenType::COMMA, ","); break;
        case ':':
            if (match('=')) addToken(TokenType::FAT_ARROW, "=>");
            else addToken(TokenType::COLON, ":");
            break;
        case ';': addToken(TokenType::SEMICOLON, ";"); break;
        case '(': addToken(TokenType::LPAREN, "("); break;
        case ')': addToken(TokenType::RPAREN, ")"); break;
        case '{': addToken(TokenType::LBRACE, "{"); break;
        case '}': addToken(TokenType::RBRACE, "}"); break;
        case '[': addToken(TokenType::LBRACK, "["); break;
        case ']': addToken(TokenType::RBRACK, "]"); break;
    }
}


// Main lexing loop: tokenizes the entire source string
std::vector<Token> Lexer::tokenize() {
    while (!isAtEnd()) {
        skipWhitespace();
        if (isAtEnd()) break;
        char c = peek();
        if (std::isalpha(c) || c == '_') { advance(); identifier(); }
        else if (std::isdigit(c)) { advance(); number(); }
        else if (c == '"') { advance(); stringLiteral(); }
        else if (c == '\'') { advance(); charLiteral(); }
        else { symbol(); }
    }
    addToken(TokenType::END_OF_FILE, "");
    return tokens;
}

void Lexer::enterScope() {
    symbolTable.enterScope();
}

void Lexer::exitScope() {
    symbolTable.exitScope();
}
