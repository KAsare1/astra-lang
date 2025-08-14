#pragma once
#include "../abstract-syntax-tree/ast.h"
#include "../shared/symbol_table.h"
#include <stdexcept>

class SemanticAnalyzer {
    SymbolTable symbols;

public:
    void analyze(const std::vector<std::unique_ptr<Stmt>>& statements) {
        for (const auto& stmt : statements) {
            analyzeStmt(stmt.get());
        }
    }

private:
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
        symbols.setType(varDecl->name, "int"); // Example: set type to "int"
        if (varDecl->initializer) {
            std::string initializerType = analyzeExpr(varDecl->initializer.get());
            if (initializerType != "int") {
                throw std::runtime_error("Type mismatch: variable '" + varDecl->name + "' is declared as 'int' but initialized with '" + initializerType + "'");
            }
        }
    }

    std::string analyzeExpr(const Expr* expr) {
        if (auto varExpr = dynamic_cast<const VariableExpr*>(expr)) {
            if (!symbols.isDeclared(varExpr->name)) {
                throw std::runtime_error("Use of undeclared variable '" + varExpr->name + "'");
            }
            return symbols.getType(varExpr->name);
        }
        else if (dynamic_cast<const LiteralExpr*>(expr)) {
            return "int"; // Example: assume all literals are integers for now
        }
        else if (auto binary = dynamic_cast<const BinaryExpr*>(expr)) {
            std::string leftType = analyzeExpr(binary->left.get());
            std::string rightType = analyzeExpr(binary->right.get());
            if (leftType != rightType) {
                throw std::runtime_error("Type mismatch in binary expression: '" + leftType + "' and '" + rightType + "'");
            }
            return leftType; // Return the resulting type
        }
        else if (auto call = dynamic_cast<const CallExpr*>(expr)) {
            if (!symbols.isDeclared(call->callee)) {
                throw std::runtime_error("Call to undeclared function '" + call->callee + "'");
            }
            return symbols.getType(call->callee); // Assume the function's return type is stored in the SymbolTable
        }
        throw std::runtime_error("Unknown expression type");
    }
};
