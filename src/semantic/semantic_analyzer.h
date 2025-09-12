#pragma once
#include "../abstract-syntax-tree/ast.h"
#include "../shared/symbol_table.h"
#include "../shared/error_handler.h"
#include <stdexcept>
#include <vector>
#include <string>

class SemanticAnalyzer {
public:
    SemanticAnalyzer(SymbolTable& symbolTable, ErrorHandler& errors);
    
    void analyze(const std::vector<std::unique_ptr<Stmt>>& statements);
    const SymbolTable& getSymbolTable() const;

private:
    SymbolTable& symbols;
    ErrorHandler& errorHandler;
    int currentLine;
    
    // NEW: Function context tracking
    std::string currentFunctionName;
    std::string currentFunctionReturnType;
    bool inFunctionBody;

    // Initialization
    void initializeBuiltins();

    // Statement analysis
    void analyzeStmt(const Stmt* stmt);
    void handleVarDecl(const VarDeclStmt* varDecl);
    void handleAssignmentStmt(const AssignmentStmt* assignStmt);
    void handleBlockStmt(const BlockStmt* blockStmt);
    void handleIfStmt(const IfStmt* ifStmt);
    void handleWhileStmt(const WhileStmt* whileStmt);
    void handleForStmt(const ForStmt* forStmt);
    void handleFunctionDecl(const FunctionDeclStmt* funcDecl);  // NEW
    void handleReturnStmt(const ReturnStmt* retStmt);          // NEW

    // Expression analysis
    std::string analyzeExpr(const Expr* expr);

    // NEW: Function-specific analysis
    void validateFunctionSignature(const std::string& name, 
                                   const std::vector<Parameter>& params,
                                   const std::string& returnType);
    void validateReturnType(const std::string& returnType);
    std::string createFunctionSignature(const std::vector<Parameter>& params, 
                                        const std::string& returnType);

    // Utility
    void checkForUnusedVariables();
    void checkForUnusedFunctions();  // NEW


    // NEW: Data structure analysis methods
    void handleIndexAssignmentStmt(const IndexAssignmentStmt* indexAssign);
    std::string analyzeArrayLiteral(const ArrayLiteralExpr* arrayLit);
    std::string analyzeIndexExpr(const IndexExpr* indexExpr);
    std::string analyzeBuiltinCall(const CallExpr* call);
    
    // NEW: Type system helpers for arrays
    std::string getArrayElementType(const std::string& arrayType);
    std::string makeArrayType(const std::string& elementType);
    bool isArrayType(const std::string& type);
    int getArrayDimensions(const std::string& arrayType);
    bool isValidArrayIndex(const std::string& indexType);
    
    // Enhanced type compatibility
    bool areTypesCompatible(const std::string& expected, const std::string& actual);
    bool canConcatenate(const std::string& leftType, const std::string& rightType);
    std::string inferBinaryResultType(const std::string& leftType, 
                                     const std::string& rightType, 
                                     const std::string& op);
    void validateTypeDeclaration(const std::string& typeDecl);
    void debugPrintType(const std::string& type, const std::string& context);
};