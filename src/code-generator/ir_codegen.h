#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "../abstract-syntax-tree/ast.h"
#include "../shared/symbol_table.h"

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
    IRCodegen(const std::string& moduleName, SymbolTable& symbols);

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

    // Current function (only 'main' for now)
    llvm::Function* currentFunction = nullptr;

    // Local variable storage (name -> alloca)
    std::unordered_map<std::string, llvm::AllocaInst*> namedValues;

    // === Helpers ===
    llvm::Function* createMain();
    llvm::AllocaInst* createEntryAlloca(llvm::Function* fn, llvm::Type* ty, const std::string& name);
    llvm::Type* inferType(const Expr* expr);
    llvm::Value* ensurePrintCall(llvm::Value* arg);
    llvm::Function* getOrCreatePrintInt();
    llvm::Function* getOrCreatePrintDouble();
    llvm::Function* getOrCreatePrintString();

    // === Stmt / Expr generators ===
    void genStmt(const Stmt* stmt);
    void genVarDecl(const VarDeclStmt* v);
    void genExprStmt(const ExprStmt* s);

    llvm::Value* genExpr(const Expr* expr);
    llvm::Value* genLiteral(const LiteralExpr* lit);
    llvm::Value* genVariable(const VariableExpr* var);
    llvm::Value* genBinary(const BinaryExpr* bin);   // placeholder; not used yet
    llvm::Value* genCall(const CallExpr* call);
};
