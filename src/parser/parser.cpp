#include "parser.h"
#include <stdexcept>
#include <memory>
#include <iostream> 

using namespace std;

Parser::Parser(const std::vector<Token>& tokens, SymbolTable& symbols, ErrorHandler& errors) 
    : tokens(tokens), symbolTable(symbols), errorHandler(errors) {}

std::vector<std::unique_ptr<Stmt>> Parser::parse() {
    std::vector<std::unique_ptr<Stmt>> statements;
    
    while (!isAtEnd()) {
    std::cerr << "[DEBUG] parse loop: current=" << current << ", token=" << peek().lexeme << " (type=" << static_cast<int>(peek().type) << ")" << std::endl;
        try {
            auto stmt = declaration();
            if (stmt) {
                statements.push_back(std::move(stmt));
            }
        } catch (const std::runtime_error& e) {
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
    } else if (type == TokenType::LBRACE) {
        errorHandler.addSuggestion("Add opening brace '{' to start block");
    } else if (type == TokenType::RBRACE) {
        errorHandler.addSuggestion("Add closing brace '}' to end block");
    }
    
    // Return current token for error recovery
    return current;
}

std::unique_ptr<Stmt> Parser::declaration() {
    try {
        if (match({TokenType::KW_LET})) {
            return varDeclaration();
        }
        if (match({TokenType::KW_FN})) {  // ADD THIS
            return functionDeclaration();
        }
        return statement();
    } catch (const std::runtime_error& e) {
        std::cerr << "Error in declaration at token position " << current << "\n";
        throw;
    }
}


std::unique_ptr<Stmt> Parser::varDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expected variable name after 'let'");
    
    // NEW: Optional type annotation
    std::string typeAnnotation = "";
    if (match({TokenType::COLON})) {
        typeAnnotation = parseTypeAnnotation();
    }
    
    std::unique_ptr<Expr> initializer = nullptr;
    if (match({TokenType::ASSIGN})) {
        initializer = expression();
    }
    
    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration");
    return std::make_unique<VarDeclStmt>(name.lexeme, typeAnnotation, std::move(initializer));
}

std::unique_ptr<Stmt> Parser::statement() {
    // Handle control flow statements
    if (match({TokenType::KW_IF})) {
        return ifStatement();
    }
    if (match({TokenType::KW_WHILE})) {
        return whileStatement();
    }
    // NEW: Handle for loops
    if (match({TokenType::KW_FOR})) {
        return forStatement();
    }
    if (match({TokenType::KW_RETURN})) {  // ADD THIS
        return returnStatement();
    }
    if (match({TokenType::LBRACE})) {
        // Put the brace back and parse as block
        current--;
        return blockStatement();
    }
    
    // Check for assignment vs expression statement
    if (check(TokenType::IDENTIFIER)) {
        size_t saved = current;
        advance(); // consume identifier
        
        if (match({TokenType::LBRACK})) {
            // This might be index assignment: arr[i] = value
            current = saved;
            return parseIndexAssignmentOrExpression();
        } else if (match({TokenType::ASSIGN})) {
            // Regular assignment: identifier = expression
            current = saved;
            return assignmentStatement();
        } else {
            // Expression statement
            current = saved;
            return expressionStatement();
        }
    }
    
    return expressionStatement();
}

std::unique_ptr<Stmt> Parser::returnStatement() {
    std::unique_ptr<Expr> value = nullptr;
    
    // Check if there's a return value
    if (!check(TokenType::SEMICOLON)) {
        value = expression();
    }
    
    consume(TokenType::SEMICOLON, "Expected ';' after return statement");
    return std::make_unique<ReturnStmt>(std::move(value));
}


std::unique_ptr<Stmt> Parser::assignmentStatement() {
    Token name = consume(TokenType::IDENTIFIER, "Expected variable name");
    consume(TokenType::ASSIGN, "Expected '=' in assignment");
    
    auto value = expression();
    consume(TokenType::SEMICOLON, "Expected ';' after assignment");
    
    return std::make_unique<AssignmentStmt>(name.lexeme, std::move(value));
}

std::unique_ptr<Stmt> Parser::ifStatement() {
    consume(TokenType::LPAREN, "Expected '(' after 'if'");
    auto condition = expression();
    consume(TokenType::RPAREN, "Expected ')' after if condition");
    
    auto thenBranch = statement();
    std::unique_ptr<Stmt> elseBranch = nullptr;
    
    if (match({TokenType::KW_ELSE})) {
        elseBranch = statement();
    }
    
    return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
}

std::unique_ptr<Stmt> Parser::whileStatement() {
    consume(TokenType::LPAREN, "Expected '(' after 'while'");
    auto condition = expression();
    consume(TokenType::RPAREN, "Expected ')' after while condition");
    
    auto body = statement();
    
    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}

// NEW: For statement parsing
std::unique_ptr<Stmt> Parser::forStatement() {
    consume(TokenType::LPAREN, "Expected '(' after 'for'");
    
    // Parse: variable_name in range_expression
    Token variable = consume(TokenType::IDENTIFIER, "Expected loop variable name");
    consume(TokenType::KW_IN, "Expected 'in' after loop variable");
    
    // Parse range expression: start:end
    auto rangeExpr = parseRangeExpression();
    
    consume(TokenType::RPAREN, "Expected ')' after for range");
    
    auto body = statement();
    
    return std::make_unique<ForStmt>(variable.lexeme, std::move(rangeExpr), std::move(body));
}

// NEW: Parse range expressions (start:end)
std::unique_ptr<Expr> Parser::parseRangeExpression() {
    auto start = expression();
    
    consume(TokenType::COLON, "Expected ':' in range expression");
    
    auto end = expression();
    
    // NEW: Check for optional step
    std::unique_ptr<Expr> step = nullptr;
    if (match({TokenType::COLON})) {
        // Parse step value: start:end:step
        step = expression();
    }
    
    // Create RangeExpr with optional step
    return std::make_unique<RangeExpr>(std::move(start), std::move(end), std::move(step));
}

std::unique_ptr<Stmt> Parser::blockStatement() {
    consume(TokenType::LBRACE, "Expected '{'");
    
    auto block = std::make_unique<BlockStmt>();
    
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        auto stmt = declaration();
        if (stmt) {
            block->statements.push_back(std::move(stmt));
        }
    }
    
    consume(TokenType::RBRACE, "Expected '}' after block");
    
    return block;
}

std::unique_ptr<Stmt> Parser::expressionStatement() {
    auto expr = expression();
    consume(TokenType::SEMICOLON, "Expected ';' after expression.");
    return std::make_unique<ExprStmt>(std::move(expr));
}

// Expression parsing with proper precedence
std::unique_ptr<Expr> Parser::expression() {
    return assignment();
}

std::unique_ptr<Expr> Parser::assignment() {
    return logicalOr();
}

std::unique_ptr<Expr> Parser::logicalOr() {
    auto expr = logicalAnd();
    
    while (match({TokenType::OR_OR})) {
        std::string op = previous().lexeme;
        auto right = logicalAnd();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::logicalAnd() {
    auto expr = equality();
    
    while (match({TokenType::AND_AND})) {
        std::string op = previous().lexeme;
        auto right = equality();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::equality() {
    auto expr = comparison();
    
    while (match({TokenType::EQ, TokenType::NEQ})) {
        std::string op = previous().lexeme;
        auto right = comparison();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::comparison() {
    auto expr = additive();
    
    while (match({TokenType::GT, TokenType::GTE, TokenType::LT, TokenType::LTE})) {
        std::string op = previous().lexeme;
        auto right = additive();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::additive() {
    auto expr = multiplicative();
    
    while (match({TokenType::PLUS, TokenType::MINUS})) {
        std::string op = previous().lexeme;
        auto right = multiplicative();
        
        // Special handling for string concatenation
        if (op == "+") {
            // This could be either arithmetic addition or string concatenation
            // Let semantic analysis decide based on types
            expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
        } else {
            expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
        }
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::multiplicative() {
    auto expr = unary();
    
    while (match({TokenType::STAR, TokenType::SLASH, TokenType::PERCENT})) {
        std::string op = previous().lexeme;
        auto right = unary();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::unary() {
    if (match({TokenType::BANG, TokenType::MINUS})) {
        std::string op = previous().lexeme;
        auto right = unary();
        return std::make_unique<UnaryExpr>(op, std::move(right));
    }
    
    return call();
}

std::unique_ptr<Expr> Parser::call() {
    // Accept both identifiers and built-in function keywords as function names
    if (!(check(TokenType::IDENTIFIER)
          || check(TokenType::KW_LEN) || check(TokenType::KW_PUSH) || check(TokenType::KW_POP)
          || check(TokenType::KW_CONCAT) || check(TokenType::KW_SUBSTR) || check(TokenType::KW_INPUT)
          || check(TokenType::KW_PRINTLN) || check(TokenType::KW_TO_STRING) || check(TokenType::KW_TO_INT) || check(TokenType::KW_TO_DOUBLE))) {
        return primary();
    }

    std::string name = advance().lexeme;

    // Handle function calls (built-in or user-defined)
    if (match({TokenType::LPAREN})) {
        auto callExpr = std::make_unique<CallExpr>(name);
        // Parse arguments if any
        if (!check(TokenType::RPAREN)) {
            do {
                callExpr->arguments.push_back(expression());
            } while (match({TokenType::COMMA}));
        }
        consume(TokenType::RPAREN, "Expected ')' after arguments.");
        return callExpr;
    }

    // This is a variable reference, not a function call
    std::unique_ptr<Expr> varExpr = std::make_unique<VariableExpr>(name);

    // Check for array indexing
    while (match({TokenType::LBRACK})) {
        auto index = expression();
        consume(TokenType::RBRACK, "Expected ']' after array index");
        varExpr = std::make_unique<IndexExpr>(std::move(varExpr), std::move(index));
    }

    return varExpr;
}


std::unique_ptr<Expr> Parser::primary() {
    // Handle boolean literals
    if (match({TokenType::KW_TRUE})) {
        return std::make_unique<LiteralExpr>("1");
    }
    
    if (match({TokenType::KW_FALSE})) {
        return std::make_unique<LiteralExpr>("0");
    }
    
    // Handle array literals [1, 2, 3]
    if (match({TokenType::LBRACK})) {
        return parseArrayLiteral();
    }
    
    // Handle regular literals
    if (match({TokenType::INT_LITERAL, TokenType::FLOAT_LITERAL, TokenType::STRING_LITERAL})) {
        return std::make_unique<LiteralExpr>(previous().lexeme);
    }
    
    // REMOVED: Don't handle identifiers here - they should be handled in call()
    
    // Handle parenthesized expressions
    if (match({TokenType::LPAREN})) {
        auto expr = expression();
        consume(TokenType::RPAREN, "Expected ')' after expression.");
        return expr;
    }
    
    // Error case
    Token current = peek();
    errorHandler.reportError(ErrorCategory::SYNTAX,
        "Expected expression", current.line, current.column, "parsing primary expression");
    errorHandler.addSuggestion("Provide a number, string, boolean, array literal, or variable name");
    
    return nullptr;
}



void Parser::synchronize() {
    advance();
    
    int syncCount = 0;
    while (!isAtEnd()) {
    std::cerr << "[DEBUG] synchronize: current=" << current << ", token=" << peek().lexeme << " (type=" << static_cast<int>(peek().type) << ")" << std::endl;
        if (previous().type == TokenType::SEMICOLON) return;
        switch (peek().type) {
            case TokenType::KW_LET:
            case TokenType::KW_FN:
            case TokenType::KW_IF:
            case TokenType::KW_WHILE:
            case TokenType::KW_FOR:
            case TokenType::KW_RETURN:
                return;
            default:
                break;
        }
        advance();
        // Prevent infinite loops
        if (++syncCount > 100) {
            errorHandler.reportFatal(ErrorCategory::SYNTAX, 
                "Parser synchronization failed - too many errors");
            break;
        }
    }
}



// NEW: Function declaration parsing
std::unique_ptr<Stmt> Parser::functionDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expected function name after 'fn'");
    
    consume(TokenType::LPAREN, "Expected '(' after function name");
    std::vector<Parameter> parameters = parseParameterList();
    consume(TokenType::RPAREN, "Expected ')' after parameters");
    
    // Parse return type
    std::string returnType = "void";  // Default return type
    if (match({TokenType::ARROW})) {
        returnType = parseTypeAnnotation();
    }
    
    // Parse function body
    consume(TokenType::LBRACE, "Expected '{' before function body");
    auto body = std::make_unique<BlockStmt>();
    
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        auto stmt = declaration();
        if (stmt) {
            body->statements.push_back(std::move(stmt));
        }
    }
    
    consume(TokenType::RBRACE, "Expected '}' after function body");
    
    return std::make_unique<FunctionDeclStmt>(name.lexeme, std::move(parameters), 
                                              returnType, std::move(body));
}

// NEW: Parse parameter list for functions
std::vector<Parameter> Parser::parseParameterList() {
    std::vector<Parameter> parameters;
    
    if (!check(TokenType::RPAREN)) {
        do {
            Token paramName = consume(TokenType::IDENTIFIER, "Expected parameter name");
            consume(TokenType::COLON, "Expected ':' after parameter name");
            std::string paramType = parseTypeAnnotation();
            
            parameters.emplace_back(paramName.lexeme, paramType);
        } while (match({TokenType::COMMA}));
    }
    
    return parameters;
}

// MODIFY parseTypeAnnotation() to support array types:
std::string Parser::parseTypeAnnotation() {
    // Check for array type syntax: [type] or [[type]]
    if (match({TokenType::LBRACK})) {
        return parseArrayType();
    }
    
    // Regular type parsing (existing code)
    if (isTypeKeyword(peek().type)) {
        Token typeToken = advance();
        return tokenTypeToTypeString(typeToken.type);
    } else {
        errorHandler.reportError(ErrorCategory::SYNTAX,
            "Expected type annotation", peek().line, peek().column, "parsing type");
        errorHandler.addSuggestion("Use a valid type like 'int', '[int]', 'string', 'bool', or 'void'");
        return "int";
    }
}


// NEW: Helper functions for type checking
bool Parser::isTypeKeyword(TokenType type) const {
    return type == TokenType::KW_INT || type == TokenType::KW_DOUBLE || 
           type == TokenType::KW_STRING || type == TokenType::KW_BOOL || 
           type == TokenType::KW_VOID;
}

std::string Parser::tokenTypeToTypeString(TokenType type) const {
    switch (type) {
        case TokenType::KW_INT: return "int";
        case TokenType::KW_DOUBLE: return "double";
        case TokenType::KW_STRING: return "string";
        case TokenType::KW_BOOL: return "bool";
        case TokenType::KW_VOID: return "void";
        default: return "unknown";
    }
}


// 2. NEW: parseArrayType() method:
std::string Parser::parseArrayType() {
    // Already consumed the opening '['
    std::string baseType;
    
    // Check for nested arrays: [[int]]
    if (check(TokenType::LBRACK)) {
        baseType = parseArrayType();  // Recursive call for nested arrays
    } else {
        // Parse base type
        if (isTypeKeyword(peek().type)) {
            Token typeToken = advance();
            baseType = tokenTypeToTypeString(typeToken.type);
        } else {
            errorHandler.reportError(ErrorCategory::SYNTAX,
                "Expected base type in array declaration", 
                peek().line, peek().column, "parsing array type");
            baseType = "int";  // Default fallback
        }
    }
    
    consume(TokenType::RBRACK, "Expected ']' after array type");
    return "[" + baseType + "]";  // Return "[int]", "[[string]]", etc.
}



std::unique_ptr<Stmt> Parser::parseIndexAssignmentOrExpression() {
    // Save the current position
    size_t savedPosition = current;
    
    // Try to parse as a regular expression first
    auto expr = expression();
    
    // Check if this is actually an assignment
    if (match({TokenType::ASSIGN})) {
        // expr should be an IndexExpr for valid index assignment
        if (auto indexExpr = dynamic_cast<IndexExpr*>(expr.get())) {
            // Extract components and create IndexAssignmentStmt
            // FIXED: Create new copies instead of releasing from unique_ptr
            auto objectCopy = std::unique_ptr<Expr>();
            auto indexCopy = std::unique_ptr<Expr>();
            
            // Create copies of the expressions
            if (auto varExpr = dynamic_cast<VariableExpr*>(indexExpr->object.get())) {
                objectCopy = std::make_unique<VariableExpr>(varExpr->name);
            }
            
            if (auto litExpr = dynamic_cast<LiteralExpr*>(indexExpr->index.get())) {
                indexCopy = std::make_unique<LiteralExpr>(litExpr->value);
            } else if (auto varExpr = dynamic_cast<VariableExpr*>(indexExpr->index.get())) {
                indexCopy = std::make_unique<VariableExpr>(varExpr->name);
            }
            
            auto value = expression();
            
            consume(TokenType::SEMICOLON, "Expected ';' after index assignment");
            return std::make_unique<IndexAssignmentStmt>(
                std::move(objectCopy), std::move(indexCopy), std::move(value));
        } else {
            errorHandler.reportError(ErrorCategory::SYNTAX,
                "Invalid left-hand side of assignment", 
                peek().line, peek().column, "parsing assignment");
            return nullptr;
        }
    }
    
    // Otherwise, it's an expression statement
    consume(TokenType::SEMICOLON, "Expected ';' after expression");
    return std::make_unique<ExprStmt>(std::move(expr));
}




std::unique_ptr<Expr> Parser::parseArrayLiteral() {
    // Already consumed the opening '['
    auto arrayExpr = std::make_unique<ArrayLiteralExpr>();
    
    // Handle empty array []
    if (match({TokenType::RBRACK})) {
        return arrayExpr;
    }
    
    // Parse array elements
    do {
        auto element = expression();
        if (element) {
            arrayExpr->elements.push_back(std::move(element));
        }
    } while (match({TokenType::COMMA}));
    
    consume(TokenType::RBRACK, "Expected ']' after array elements");
    return arrayExpr;
}


bool Parser::isBuiltinFunction(const std::string& name) const {
    return name == "len" || name == "push" || name == "pop" || 
           name == "concat" || name == "substr" || name == "input" || 
           name == "println" || name == "to_string" || name == "to_int" || 
           name == "to_double";
}

// std::unique_ptr<Expr> Parser::parseBuiltinFunction(const std::string& name) {
//     // DON'T consume LPAREN here - it was already consumed in call()
    
//     auto callExpr = std::make_unique<CallExpr>(name);
    
//     if (!check(TokenType::RPAREN)) {
//         do {
//             callExpr->arguments.push_back(expression());
//         } while (match({TokenType::COMMA}));
//     }
    
//     consume(TokenType::RPAREN, "Expected ')' after built-in function arguments");
//     return callExpr;
// }