#include "parser.h"
#include <stdexcept>
#include <memory>
#include <iostream> // Include iostream for debugging logs

Parser::Parser(const std::vector<Token>& tokens, SymbolTable& symbols, ErrorHandler& errors) 
    : tokens(tokens), symbolTable(symbols), errorHandler(errors) {}

std::vector<std::unique_ptr<Stmt>> Parser::parse() {
    std::vector<std::unique_ptr<Stmt>> statements;
    
    while (!isAtEnd()) {
        try {
            auto stmt = declaration();
            if (stmt) {  // Only add non-null statements
                statements.push_back(std::move(stmt));
            }
        } catch (const std::runtime_error& e) {
            // Should rarely happen now since we use errorHandler
            synchronize();
            if (errorHandler.getErrorCount() > 20) {
                errorHandler.reportFatal(ErrorCategory::SYNTAX, 
                    "Too many parse errors, stopping compilation");
                break;
            }
        }
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
    
    // Enhanced error reporting with location
    Token current = peek();
    errorHandler.reportError(ErrorCategory::SYNTAX, errorMessage, 
                            current.line, current.column,
                            "parsing statement");
    
    // Add context-specific suggestions
    if (type == TokenType::SEMICOLON) {
        errorHandler.addSuggestion("Add semicolon (;) at the end of the statement");
    } else if (type == TokenType::IDENTIFIER) {
        errorHandler.addSuggestion("Provide a valid identifier name");
    } else if (type == TokenType::ASSIGN) {
        errorHandler.addSuggestion("Use '=' to assign a value");
    }
    
    // Return current token for error recovery
    return current;
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
    // Remove debug output, use error handler instead
    Token name = consume(TokenType::IDENTIFIER, "Expected variable name after 'let'");
    
    if (symbolTable.isDeclaredInCurrentScope(name.lexeme)) {
        errorHandler.reportError(ErrorCategory::SEMANTIC,
            "Variable '" + name.lexeme + "' is already declared in this scope",
            name.line, name.column, "declaring variable");
        errorHandler.addSuggestion("Use a different variable name");
        errorHandler.addSuggestion("Remove the duplicate declaration");
        // Continue parsing to find more errors
        return nullptr;  // Return null to skip this declaration
    }
    
    symbolTable.declare(name.lexeme, SymbolKind::VARIABLE);
    
    std::unique_ptr<Expr> initializer = nullptr;
    if (match({TokenType::ASSIGN})) {
        initializer = expression();
        if (initializer) {  // Only mark initialized if expression parsing succeeded
            symbolTable.markInitialized(name.lexeme);
        }
    }
    
    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration");
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
        if (!symbolTable.isDeclared(name.lexeme)) {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "Variable '" + name.lexeme + "' is not declared",
                name.line, name.column, "parsing expression");
            // Could add suggestions here for similar variable names
            return nullptr;  // Return null for error recovery
        }
        
        symbolTable.markUsed(name.lexeme);
        return std::make_unique<VariableExpr>(name.lexeme);
    }
    
    Token current = peek();
    errorHandler.reportError(ErrorCategory::SYNTAX,
        "Expected expression", current.line, current.column, "parsing primary expression");
    errorHandler.addSuggestion("Provide a number, string, or variable name");
    
    return nullptr;  // Error recovery
}

void Parser::enterScope() {
    symbolTable.enterScope();
}

void Parser::exitScope() {
    symbolTable.exitScope();
}


void Parser::synchronize() {
    advance();
    
    while (!isAtEnd()) {
        if (previous().type == TokenType::SEMICOLON) return;
        
        switch (peek().type) {
            case TokenType::KW_LET:
                return;
            default:
                break;
        }
        
        advance();
    }
}
