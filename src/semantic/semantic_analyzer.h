// === semantic_analyzer.h changes ===
#pragma once
#include "../abstract-syntax-tree/ast.h"
#include "../shared/symbol_table.h"
#include <stdexcept>
#include <vector>

class SemanticAnalyzer {
private:
    SymbolTable& symbols;  // Reference to shared symbol table

public:
    SemanticAnalyzer(SymbolTable& symbolTable) : symbols(symbolTable) {
        initializeBuiltins();
    }

    void analyze(const std::vector<std::unique_ptr<Stmt>>& statements) {
        for (const auto& stmt : statements) {
            analyzeStmt(stmt.get());
        }
        
        // Phase 3: Semantic Analysis - Check for unused variables
        checkForUnusedVariables();
    }

    const SymbolTable& getSymbolTable() const {
        return symbols;
    }

private:
    void initializeBuiltins() {
        // Phase 3: Declare built-in functions with proper attributes
        symbols.declare("print", SymbolKind::FUNCTION);
        symbols.setType("print", "void(any)"); // Accepts any type due to overloading
        symbols.markUsed("print"); // Built-ins are always considered "used"
    }

    void analyzeStmt(const Stmt* stmt) {
        if (auto varDecl = dynamic_cast<const VarDeclStmt*>(stmt)) {
            handleVarDecl(varDecl);
        }
        else if (auto exprStmt = dynamic_cast<const ExprStmt*>(stmt)) {
            analyzeExpr(exprStmt->expression.get());
        }
        // Future: handle blocks, functions, control flow...
    }

    void handleVarDecl(const VarDeclStmt* varDecl) {
        // Phase 3: Semantic Analysis - Type inference and validation
        if (varDecl->initializer) {
            std::string initializerType = analyzeExpr(varDecl->initializer.get());
            symbols.setType(varDecl->name, initializerType);
            
            // Variable is already marked as initialized by parser if it has initializer
        } else {
            // Default type for uninitialized variables
            symbols.setType(varDecl->name, "int");
        }
    }

    std::string analyzeExpr(const Expr* expr) {
        if (auto varExpr = dynamic_cast<const VariableExpr*>(expr)) {
            if (!symbols.isDeclared(varExpr->name)) {
                throw std::runtime_error("Use of undeclared variable '" + varExpr->name + "'");
            }
            
            // Check if variable is initialized before use
            const Symbol* symbol = symbols.getSymbol(varExpr->name);
            if (symbol && symbol->kind == SymbolKind::VARIABLE && !symbol->isInitialized) {
                std::cerr << "Warning: Variable '" << varExpr->name << "' used before initialization\n";
            }
            
            // Variable usage is already marked by parser
            return symbols.getType(varExpr->name);
        }
        else if (auto literal = dynamic_cast<const LiteralExpr*>(expr)) {
            // Phase 3: Type inference from literal content
            const std::string& v = literal->value;
            bool hasDot = false, allDigitsOrDot = !v.empty();
            for (char c : v) {
                if (c == '.') { hasDot = true; continue; }
                if (c < '0' || c > '9') { allDigitsOrDot = false; break; }
            }
            if (allDigitsOrDot && hasDot) return "double";
            if (allDigitsOrDot && !hasDot) return "int";
            return "string"; // Everything else is a string
        }
        else if (auto binary = dynamic_cast<const BinaryExpr*>(expr)) {
            std::string leftType = analyzeExpr(binary->left.get());
            std::string rightType = analyzeExpr(binary->right.get());
            if (leftType != rightType) {
                throw std::runtime_error("Type mismatch in binary expression: '" + leftType + "' and '" + rightType + "'");
            }
            return leftType;
        }
        else if (auto call = dynamic_cast<const CallExpr*>(expr)) {
            if (!symbols.isDeclared(call->callee)) {
                throw std::runtime_error("Call to undeclared function '" + call->callee + "'");
            }
            
            // Mark function as used
            symbols.markUsed(call->callee);
            
            // Phase 3: Function call validation
            if (call->callee == "print") {
                if (call->arguments.size() != 1) {
                    throw std::runtime_error("print() expects exactly one argument");
                }
                // Analyze the argument to ensure it's valid
                analyzeExpr(call->arguments[0].get());
                return "void";
            }
            
            return symbols.getType(call->callee);
        }
        throw std::runtime_error("Unknown expression type");
    }
    
    void checkForUnusedVariables() {
        auto unusedVars = symbols.getUnusedVariables();
        for (const std::string& varName : unusedVars) {
            std::cerr << "Warning: Variable '" << varName << "' declared but never used\n";
        }
    }
};