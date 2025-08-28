#pragma once
#include "../abstract-syntax-tree/ast.h"
#include "../shared/symbol_table.h"
#include <stdexcept>
#include <vector>

class SemanticAnalyzer {
    SymbolTable symbols;

public:
    SemanticAnalyzer() {
        initializeBuiltins();
    }

    void analyze(const std::vector<std::unique_ptr<Stmt>>& statements) {
        for (const auto& stmt : statements) {
            analyzeStmt(stmt.get());
        }
    }

    const SymbolTable& getSymbolTable() const {
        return symbols;
    }

private:
    void initializeBuiltins() {
        // Declare print (the function your code actually calls)
        symbols.declare("print");
        symbols.setType("print", "void(any)"); // Accepts any type due to overloading
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
        symbols.declare(varDecl->name);
        
        // Infer type from initializer if present
        if (varDecl->initializer) {
            std::string initializerType = analyzeExpr(varDecl->initializer.get());
            symbols.setType(varDecl->name, initializerType);
        } else {
            symbols.setType(varDecl->name, "int"); // Default type
        }
    }

    std::string analyzeExpr(const Expr* expr) {
        if (auto varExpr = dynamic_cast<const VariableExpr*>(expr)) {
            if (!symbols.isDeclared(varExpr->name)) {
                throw std::runtime_error("Use of undeclared variable '" + varExpr->name + "'");
            }
            return symbols.getType(varExpr->name);
        }
        else if (auto literal = dynamic_cast<const LiteralExpr*>(expr)) {
            // Infer type from literal content (similar to your IR codegen logic)
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
            
            // For print function, validate arguments but return void
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
};