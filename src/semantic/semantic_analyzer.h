#pragma once
#include "../abstract-syntax-tree/ast.h"
#include "../shared/symbol_table.h"
#include "../shared/error_handler.h"
#include <stdexcept>
#include <vector>

class SemanticAnalyzer {
private:
    SymbolTable& symbols;
    ErrorHandler& errorHandler;

public:
    SemanticAnalyzer(SymbolTable& symbolTable, ErrorHandler& errors) 
        : symbols(symbolTable), errorHandler(errors) {
        initializeBuiltins();
    }

    void analyze(const std::vector<std::unique_ptr<Stmt>>& statements) {
        for (const auto& stmt : statements) {
            if (stmt) {  // Only analyze non-null statements
                analyzeStmt(stmt.get());
            }
        }
        
        // Phase 3: Check for unused variables (as warnings)
        checkForUnusedVariables();
    }

    const SymbolTable& getSymbolTable() const {
        return symbols;
    }

private:
    void initializeBuiltins() {
        symbols.declare("print", SymbolKind::FUNCTION);
        symbols.setType("print", "void(any)");
        symbols.markUsed("print");
    }

    void analyzeStmt(const Stmt* stmt) {
        if (auto varDecl = dynamic_cast<const VarDeclStmt*>(stmt)) {
            handleVarDecl(varDecl);
        }
        else if (auto exprStmt = dynamic_cast<const ExprStmt*>(stmt)) {
            analyzeExpr(exprStmt->expression.get());
        }
    }

    void handleVarDecl(const VarDeclStmt* varDecl) {
        if (varDecl->initializer) {
            std::string initializerType = analyzeExpr(varDecl->initializer.get());
            if (initializerType != "error") {  // Only set type if analysis succeeded
                symbols.setType(varDecl->name, initializerType);
            }
        } else {
            symbols.setType(varDecl->name, "int");
        }
    }

    std::string analyzeExpr(const Expr* expr) {
        if (!expr) return "error";  // Handle null expressions from parser errors
        
        if (auto varExpr = dynamic_cast<const VariableExpr*>(expr)) {
            if (!symbols.isDeclared(varExpr->name)) {
                errorHandler.reportError(ErrorCategory::SEMANTIC,
                    "Use of undeclared variable '" + varExpr->name + "'",
                    0, 0, "analyzing expression");
                
                // Could add variable name suggestions here
                return "error";
            }
            
            // Check if variable is initialized before use
            const Symbol* symbol = symbols.getSymbol(varExpr->name);
            if (symbol && symbol->kind == SymbolKind::VARIABLE && !symbol->isInitialized) {
                errorHandler.reportWarning(ErrorCategory::SEMANTIC,
                    "Variable '" + varExpr->name + "' may be used before initialization",
                    0, 0, "analyzing variable usage");
                errorHandler.addSuggestion("Initialize the variable before using it");
            }
            
            return symbols.getType(varExpr->name);
        }
        else if (auto literal = dynamic_cast<const LiteralExpr*>(expr)) {
            const std::string& v = literal->value;
            bool hasDot = false, allDigitsOrDot = !v.empty();
            for (char c : v) {
                if (c == '.') { hasDot = true; continue; }
                if (c < '0' || c > '9') { allDigitsOrDot = false; break; }
            }
            if (allDigitsOrDot && hasDot) return "double";
            if (allDigitsOrDot && !hasDot) return "int";
            return "string";
        }
        else if (auto binary = dynamic_cast<const BinaryExpr*>(expr)) {
            std::string leftType = analyzeExpr(binary->left.get());
            std::string rightType = analyzeExpr(binary->right.get());
            
            if (leftType == "error" || rightType == "error") {
                return "error";  // Propagate errors
            }
            
            if (leftType != rightType) {
                errorHandler.reportError(ErrorCategory::SEMANTIC,
                    "Type mismatch in binary expression: '" + leftType + "' and '" + rightType + "'",
                    0, 0, "analyzing binary expression");
                return "error";
            }
            return leftType;
        }
        else if (auto call = dynamic_cast<const CallExpr*>(expr)) {
            if (!symbols.isDeclared(call->callee)) {
                errorHandler.reportError(ErrorCategory::SEMANTIC,
                    "Call to undeclared function '" + call->callee + "'",
                    0, 0, "analyzing function call");
                return "error";
            }
            
            symbols.markUsed(call->callee);
            
            if (call->callee == "print") {
                if (call->arguments.size() != 1) {
                    errorHandler.reportError(ErrorCategory::SEMANTIC,
                        "print() expects exactly one argument, got " + std::to_string(call->arguments.size()),
                        0, 0, "analyzing function call");
                    errorHandler.addSuggestion("Provide exactly one argument to print()");
                    return "error";
                }
                
                // Analyze the argument
                std::string argType = analyzeExpr(call->arguments[0].get());
                return "void";
            }
            
            return symbols.getType(call->callee);
        }
        
        errorHandler.reportError(ErrorCategory::SEMANTIC,
            "Unknown expression type in semantic analysis",
            0, 0, "analyzing expression");
        return "error";
    }
    
    void checkForUnusedVariables() {
        auto unusedVars = symbols.getUnusedVariables();
        for (const std::string& varName : unusedVars) {
            errorHandler.reportWarning(ErrorCategory::SEMANTIC,
                "Variable '" + varName + "' declared but never used",
                0, 0, "checking variable usage");
            errorHandler.addSuggestion("Remove the unused variable or use it in your code");
        }
    }
};