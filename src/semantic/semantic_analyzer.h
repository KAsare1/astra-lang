#pragma once
#include "../abstract-syntax-tree/ast.h"
#include "../shared/symbol_table.h"
#include "../shared/error_handler.h"
#include <stdexcept>
#include <vector>

class SemanticAnalyzer {
public:
    SemanticAnalyzer(SymbolTable& symbolTable, ErrorHandler& errors);
    
    void analyze(const std::vector<std::unique_ptr<Stmt>>& statements);
    const SymbolTable& getSymbolTable() const;

private:
    SymbolTable& symbols;
    ErrorHandler& errorHandler;
    int currentLine;

    // Initialization
    void initializeBuiltins();

    // Statement analysis
    void analyzeStmt(const Stmt* stmt);
    void handleVarDecl(const VarDeclStmt* varDecl);
    void handleAssignmentStmt(const AssignmentStmt* assignStmt);
    void handleBlockStmt(const BlockStmt* blockStmt);
    void handleIfStmt(const IfStmt* ifStmt);
    void handleWhileStmt(const WhileStmt* whileStmt);
    void handleForStmt(const ForStmt* forStmt);  // NEW: For loop handler

    // Expression analysis
    std::string analyzeExpr(const Expr* expr);

    // Utility
    void checkForUnusedVariables();
};