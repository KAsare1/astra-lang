#pragma once
#include "../abstract-syntax-tree/ast.h"
#include "symbol_table.h"
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
        if (varDecl->initializer) {
            analyzeExpr(varDecl->initializer.get());
        }
    }

    void analyzeExpr(const Expr* expr) {
        if (auto varExpr = dynamic_cast<const VariableExpr*>(expr)) {
            if (!symbols.isDeclared(varExpr->name)) {
                throw std::runtime_error("Use of undeclared variable '" + varExpr->name + "'");
            }
        }
        else if (auto literal = dynamic_cast<const LiteralExpr*>(expr)) {
            (void)literal; // nothing to check for literals yet
        }
        else if (auto binary = dynamic_cast<const BinaryExpr*>(expr)) {
            analyzeExpr(binary->left.get());
            analyzeExpr(binary->right.get());
        }
        else if (auto call = dynamic_cast<const CallExpr*>(expr)) {
            if (!symbols.isDeclared(call->callee)) {
                throw std::runtime_error("Call to undeclared function '" + call->callee + "'");
            }
            for (auto& arg : call->arguments) {
                analyzeExpr(arg.get());
            }
        }
    }
};
