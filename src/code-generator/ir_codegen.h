#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "../abstract-syntax-tree/ast.h"
#include "../shared/symbol_table.h"
#include "../shared/error_handler.h"

// LLVM headers
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalValue.h"

class IRCodegen {
public:
    IRCodegen(const std::string& moduleName, SymbolTable& symbols, ErrorHandler& errors);

    // Generate IR for a whole program (sequence of statements)
    void emit(const std::vector<std::unique_ptr<Stmt>>& statements);

    // Access the module to print or write to file
    llvm::Module& getModule() { return *module; }
    llvm::LLVMContext& getContext() { return context; }

private:
    // Core LLVM state
    llvm::LLVMContext context;
    std::unique_ptr<llvm::Module> module;
    llvm::IRBuilder<> builder;

    // Reference to shared symbol table and error handler
    SymbolTable& symbolTable;
    ErrorHandler& errorHandler;

    // Function management
    llvm::Function* currentFunction = nullptr;
    std::unordered_map<std::string, llvm::Function*> functions;
    
    // Function return handling
    std::string currentFunctionReturnType;
    llvm::BasicBlock* functionExitBlock = nullptr;
    llvm::AllocaInst* returnValue = nullptr;

    // Local variable storage (name -> alloca)
    std::unordered_map<std::string, llvm::AllocaInst*> namedValues;

    // NEW: Array and string management
    std::unordered_map<std::string, llvm::StructType*> arrayTypes;     // Cache for array struct types
    std::unordered_map<std::string, llvm::Function*> runtimeFunctions; // Built-in function cache

    // === Core Helpers ===
    llvm::Function* createMain();
    llvm::AllocaInst* createEntryAlloca(llvm::Function* fn, llvm::Type* ty, const std::string& name);
    llvm::Type* inferType(const Expr* expr);
    llvm::Value* ensurePrintCall(llvm::Value* arg);
    llvm::Function* getOrCreatePrintInt();
    llvm::Function* getOrCreatePrintDouble();
    llvm::Function* getOrCreatePrintString();
    llvm::Value* createCondition(llvm::Value* value);
    llvm::Type* getTypeFromSymbolTable(const std::string& name);

    // === Enhanced Type System ===
    llvm::Type* stringToLLVMType(const std::string& typeStr);  // Enhanced for arrays
    bool isArrayTypeString(const std::string& typeStr);
    std::string getArrayElementTypeString(const std::string& arrayTypeStr);
    llvm::StructType* getOrCreateStringType();

    // === Function-related helpers ===
    llvm::Function* createFunction(const std::string& name, 
                                   const std::vector<Parameter>& params,
                                   const std::string& returnType);
    llvm::FunctionType* createFunctionType(const std::vector<Parameter>& params,
                                           const std::string& returnType);
    void setupFunctionEntry(llvm::Function* function, const std::vector<Parameter>& params);

    // === Array Operations ===
    llvm::StructType* getOrCreateArrayType(llvm::Type* elementType);
    llvm::Value* createArrayAllocation(llvm::Type* elementType, llvm::Value* size);
    llvm::Value* createArrayAccess(llvm::Value* arrayPtr, llvm::Value* index);
    llvm::Value* createArrayAssignment(llvm::Value* arrayPtr, llvm::Value* index, llvm::Value* value);
    
    // === String Operations ===
    llvm::Value* createStringLiteral(const std::string& str);
    llvm::Value* createStringConcat(llvm::Value* left, llvm::Value* right);
    llvm::Value* createStringAccess(llvm::Value* stringPtr, llvm::Value* index);
    llvm::Value* createStringAssignment(llvm::Value* stringPtr, llvm::Value* index, llvm::Value* value);
    llvm::Value* createSingleCharString(llvm::Value* charValue);

    // === Built-in Function Management ===
    bool isBuiltinFunction(const std::string& name);
    llvm::Value* genBuiltinCall(const CallExpr* call);
    
    // Memory management
    llvm::Function* getOrCreateMalloc();
    llvm::Function* getOrCreateFree();
    
    // Array built-ins
    llvm::Function* getOrCreateArrayPushFunction();
    llvm::Function* getOrCreateArrayPopFunction();
    
    // String built-ins
    llvm::Function* getOrCreateStringConcatFunction();
    llvm::Function* getOrCreateStringSubstrFunction();
    
    // I/O built-ins
    llvm::Function* getOrCreateInputFunction();
    llvm::Function* getOrCreatePrintlnFunction();
    
    // Type conversion built-ins
    llvm::Function* getOrCreateToStringFunction();
    llvm::Function* getOrCreateToIntFunction();
    llvm::Function* getOrCreateToDoubleFunction();
    
    // === Runtime Safety ===
    void createBoundsCheck(llvm::Value* array, llvm::Value* index, const std::string& context);
    llvm::Function* getOrCreateBoundsCheckFunction();
    llvm::Function* getOrCreateErrorExit();

    // === Statement Generators ===
    void genStmt(const Stmt* stmt);
    void genVarDecl(const VarDeclStmt* v);
    void genExprStmt(const ExprStmt* s);
    void genAssignmentStmt(const AssignmentStmt* assignStmt);
    void genIndexAssignmentStmt(const IndexAssignmentStmt* indexAssign);  // NEW
    void genBlockStmt(const BlockStmt* block);
    void genIfStmt(const IfStmt* ifStmt);
    void genWhileStmt(const WhileStmt* whileStmt);
    void genForStmt(const ForStmt* forStmt);
    void genFunctionDecl(const FunctionDeclStmt* funcDecl);
    void genReturnStmt(const ReturnStmt* retStmt);

    // === Expression Generators ===
    llvm::Value* genExpr(const Expr* expr);
    llvm::Value* genLiteral(const LiteralExpr* lit);
    llvm::Value* genVariable(const VariableExpr* var);
    llvm::Value* genUnary(const UnaryExpr* unary);
    llvm::Value* genBinary(const BinaryExpr* bin);
    llvm::Value* genCall(const CallExpr* call);
    llvm::Value* genArrayLiteral(const ArrayLiteralExpr* arrayLit);    // NEW
    llvm::Value* genIndexExpr(const IndexExpr* indexExpr);            // NEW
    llvm::Value* genRange(const RangeExpr* rangeExpr);                // Existing
};