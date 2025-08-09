#pragma once
#include "ast.h"
#include <iostream>
#include <string>
#include <memory>

inline void printExpr(const Expr* expr, int indent = 0);
inline void printStmt(const Stmt* stmt, int indent = 0);

inline void printIndent(int indent) {
    for (int i = 0; i < indent; ++i) std::cout << "  ";
}

inline void printExpr(const Expr* expr, int indent) {
    if (!expr) {
        printIndent(indent); std::cout << "(null expr)\n";
        return;
    }
    if (auto lit = dynamic_cast<const LiteralExpr*>(expr)) {
        printIndent(indent); std::cout << "LiteralExpr: " << lit->value << "\n";
    } else if (auto var = dynamic_cast<const VariableExpr*>(expr)) {
        printIndent(indent); std::cout << "VariableExpr: " << var->name << "\n";
    } else if (auto bin = dynamic_cast<const BinaryExpr*>(expr)) {
        printIndent(indent); std::cout << "BinaryExpr: " << bin->op << "\n";
        printExpr(bin->left.get(), indent + 1);
        printExpr(bin->right.get(), indent + 1);
    } else if (auto call = dynamic_cast<const CallExpr*>(expr)) {
        printIndent(indent); std::cout << "CallExpr: " << call->callee << "\n";
        for (const auto& arg : call->arguments) {
            printExpr(arg.get(), indent + 1);
        }
    } else {
        printIndent(indent); std::cout << "Unknown Expr\n";
    }
}

inline void printStmt(const Stmt* stmt, int indent) {
    if (!stmt) {
        printIndent(indent); std::cout << "(null stmt)\n";
        return;
    }
    if (auto var = dynamic_cast<const VarDeclStmt*>(stmt)) {
        printIndent(indent); std::cout << "VarDeclStmt: " << var->name << "\n";
        printExpr(var->initializer.get(), indent + 1);
    } else if (auto expr = dynamic_cast<const ExprStmt*>(stmt)) {
        printIndent(indent); std::cout << "ExprStmt\n";
        printExpr(expr->expression.get(), indent + 1);
    } else {
        printIndent(indent); std::cout << "Unknown Stmt\n";
    }
}
