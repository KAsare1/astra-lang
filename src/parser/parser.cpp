#include "parser.h"
#include <stdexcept>
#include <memory>
#include <iostream> // Include iostream for debugging logs

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens) {}

std::vector<std::unique_ptr<Stmt>> Parser::parse() {
    std::vector<std::unique_ptr<Stmt>> statements;
    while (!isAtEnd()) {
        statements.push_back(declaration());
    }
    return statements;
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::END_OF_FILE;
}

const Token& Parser::peek() const { 
    return tokens[current]; 
}

const Token& Parser::previous() const { 
    return tokens[current - 1]; 
}

const Token& Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

bool Parser::check(TokenType type) const {
    return !isAtEnd() && peek().type == type;
}

bool Parser::match(std::initializer_list<TokenType> types) {
    for (auto type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& errorMessage) {
    if (check(type)) {
        return advance();
    }
    // Debugging log to identify the issue
    std::cerr << "Error: " << errorMessage << "\n";
    std::cerr << "Current token: '" << peek().lexeme << "' (type: " << static_cast<int>(peek().type) << ")\n";
    std::cerr << "Expected token type: " << static_cast<int>(type) << "\n";
    
    // Let's also print some context
    std::cerr << "Parser context - current position: " << current << "\n";
    if (current > 0) {
        std::cerr << "Previous token: '" << previous().lexeme << "' (type: " << static_cast<int>(previous().type) << ")\n";
    }
    
    throw std::runtime_error(errorMessage);
}

std::unique_ptr<Stmt> Parser::declaration() {
    try {
        if (match({TokenType::KW_LET})) {
            return varDeclaration();
        }
        return statement();
    } catch (const std::runtime_error& e) {
        // Add more context to the error
        std::cerr << "Error in declaration at token position " << current << "\n";
        throw;
    }
}

std::unique_ptr<Stmt> Parser::varDeclaration() {
    std::cerr << "Debug: Starting variable declaration\n";
    std::cerr << "Debug: Current token before consuming identifier: '" << peek().lexeme << "' (type: " << static_cast<int>(peek().type) << ")\n";
    
    Token name = consume(TokenType::IDENTIFIER, "Expected variable name.");
    std::cerr << "Debug: Variable name: '" << name.lexeme << "'\n";
    
    // Check if the variable is already declared in the current scope
    if (symbolTable.isDeclared(name.lexeme)) {
        throw std::runtime_error("Variable '" + name.lexeme + "' is already declared in this scope.");
    }
    
    // Add the variable to the symbol table
    symbolTable.declare(name.lexeme);
    
    std::unique_ptr<Expr> initializer = nullptr;
    
    std::cerr << "Debug: Current token before checking for '=': '" << peek().lexeme << "' (type: " << static_cast<int>(peek().type) << ")\n";
    
    if (match({TokenType::ASSIGN})) {
        std::cerr << "Debug: Found '=', parsing initializer\n";
        initializer = expression();
        std::cerr << "Debug: Finished parsing initializer\n";
    }
    
    std::cerr << "Debug: Current token before consuming semicolon: '" << peek().lexeme << "' (type: " << static_cast<int>(peek().type) << ")\n";
    
    // Ensure the semicolon is consumed only after handling the initializer
    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration.");
    
    std::cerr << "Debug: Successfully parsed variable declaration\n";
    return std::make_unique<VarDeclStmt>(name.lexeme, std::move(initializer));
}

std::unique_ptr<Stmt> Parser::statement() {
    auto expr = expression();
    consume(TokenType::SEMICOLON, "Expected ';' after expression.");
    return std::make_unique<ExprStmt>(std::move(expr));
}

std::unique_ptr<Expr> Parser::expression() {
    return call();
}

std::unique_ptr<Expr> Parser::call() {
    if (!check(TokenType::IDENTIFIER)) {
        return primary();
    }
    
    std::string calleeName = advance().lexeme;
    
    if (!match({TokenType::LPAREN})) {
        // This is a variable reference, not a function call
        // Check if the variable is declared
        if (!symbolTable.isDeclared(calleeName)) {
            throw std::runtime_error("Variable '" + calleeName + "' is not declared.");
        }
        return std::make_unique<VariableExpr>(calleeName);
    }
    
    auto callExpr = std::make_unique<CallExpr>(calleeName);
    
    if (!check(TokenType::RPAREN)) {
        do {
            callExpr->arguments.push_back(expression());
        } while (match({TokenType::COMMA}));
    }
    
    if (!match({TokenType::RPAREN})) {
        throw std::runtime_error("Expected ')' after arguments.");
    }
    
    return callExpr;
}

std::unique_ptr<Expr> Parser::primary() {
    if (match({TokenType::INT_LITERAL, TokenType::FLOAT_LITERAL, TokenType::STRING_LITERAL})) {
        return std::make_unique<LiteralExpr>(previous().lexeme);
    }
    
    if (match({TokenType::IDENTIFIER})) {
        Token name = previous();
        // Check if the variable is declared
        if (!symbolTable.isDeclared(name.lexeme)) {
            throw std::runtime_error("Variable '" + name.lexeme + "' is not declared.");
        }
        return std::make_unique<VariableExpr>(name.lexeme);
    }
    
    throw std::runtime_error("Expected expression.");
}

void Parser::enterScope() {
    symbolTable.enterScope();
}

void Parser::exitScope() {
    symbolTable.exitScope();
}