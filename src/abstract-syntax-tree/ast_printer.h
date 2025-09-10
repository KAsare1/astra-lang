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
    } else if (auto unary = dynamic_cast<const UnaryExpr*>(expr)) {
        printIndent(indent); std::cout << "UnaryExpr: " << unary->op << "\n";
        printExpr(unary->operand.get(), indent + 1);
    } else if (auto call = dynamic_cast<const CallExpr*>(expr)) {
        printIndent(indent); std::cout << "CallExpr: " << call->callee << "\n";
        for (const auto& arg : call->arguments) {
            printExpr(arg.get(), indent + 1);
        }
    } else if (auto range = dynamic_cast<const RangeExpr*>(expr)) {
        printIndent(indent); std::cout << "RangeExpr\n";
        printIndent(indent + 1); std::cout << "Start:\n";
        printExpr(range->start.get(), indent + 2);
        printIndent(indent + 1); std::cout << "End:\n";
        printExpr(range->end.get(), indent + 2);
        if (range->step) {
            printIndent(indent + 1); std::cout << "Step:\n";
            printExpr(range->step.get(), indent + 2);
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
        if (var->initializer) {
            printExpr(var->initializer.get(), indent + 1);
        }
    } else if (auto expr = dynamic_cast<const ExprStmt*>(stmt)) {
        printIndent(indent); std::cout << "ExprStmt\n";
        printExpr(expr->expression.get(), indent + 1);
    } else if (auto assign = dynamic_cast<const AssignmentStmt*>(stmt)) {
        printIndent(indent); std::cout << "AssignmentStmt: " << assign->name << " =\n";
        printExpr(assign->value.get(), indent + 1);
    } else if (auto block = dynamic_cast<const BlockStmt*>(stmt)) {
        printIndent(indent); std::cout << "BlockStmt {\n";
        for (const auto& statement : block->statements) {
            printStmt(statement.get(), indent + 1);
        }
        printIndent(indent); std::cout << "}\n";
    } else if (auto ifStmt = dynamic_cast<const IfStmt*>(stmt)) {
        printIndent(indent); std::cout << "IfStmt\n";
        printIndent(indent + 1); std::cout << "Condition:\n";
        printExpr(ifStmt->condition.get(), indent + 2);
        printIndent(indent + 1); std::cout << "Then:\n";
        printStmt(ifStmt->thenBranch.get(), indent + 2);
        if (ifStmt->elseBranch) {
            printIndent(indent + 1); std::cout << "Else:\n";
            printStmt(ifStmt->elseBranch.get(), indent + 2);
        }
    } else if (auto whileStmt = dynamic_cast<const WhileStmt*>(stmt)) {
        printIndent(indent); std::cout << "WhileStmt\n";
        printIndent(indent + 1); std::cout << "Condition:\n";
        printExpr(whileStmt->condition.get(), indent + 2);
        printIndent(indent + 1); std::cout << "Body:\n";
        printStmt(whileStmt->body.get(), indent + 2);
    } else if (auto forStmt = dynamic_cast<const ForStmt*>(stmt)) {
        printIndent(indent); std::cout << "ForStmt: " << forStmt->variable << "\n";
        printIndent(indent + 1); std::cout << "Range:\n";
        printExpr(forStmt->range.get(), indent + 2);
        printIndent(indent + 1); std::cout << "Body:\n";
        printStmt(forStmt->body.get(), indent + 2);
    } else {
        printIndent(indent); std::cout << "Unknown Stmt\n";
    }
}