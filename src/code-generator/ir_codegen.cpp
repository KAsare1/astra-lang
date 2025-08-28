#include "ir_codegen.h"
#include <stdexcept>
#include <cstdlib>
#include "../shared/error_handler.h"

// ===== IRCodegen =====

IRCodegen::IRCodegen(const std::string& moduleName, SymbolTable& syms, ErrorHandler& errors)
    : module(std::make_unique<llvm::Module>(moduleName, context)),
      builder(context), symbolTable(syms), errorHandler(errors) {}

// Create: define i32 @main()
llvm::Function* IRCodegen::createMain() {
    if (currentFunction) return currentFunction;

    auto *i32Ty = llvm::Type::getInt32Ty(context);
    auto *fnTy  = llvm::FunctionType::get(i32Ty, /*isVarArg=*/false);
    auto *fn    = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage, "main", module.get());

    auto *entry = llvm::BasicBlock::Create(context, "entry", fn);
    builder.SetInsertPoint(entry);

    currentFunction = fn;
    return fn;
}

llvm::AllocaInst* IRCodegen::createEntryAlloca(llvm::Function* fn, llvm::Type* ty, const std::string& name) {
    llvm::IRBuilder<> tmp(&fn->getEntryBlock(), fn->getEntryBlock().begin());
    return tmp.CreateAlloca(ty, nullptr, name);
}

// Phase 4: Use symbol table information for type decisions
llvm::Type* IRCodegen::getTypeFromSymbolTable(const std::string& name) {
    if (symbolTable.isDeclared(name)) {
        std::string typeStr = symbolTable.getType(name);
        llvm::Type* llvmType = stringToLLVMType(typeStr);
        
        // Phase 4: Store LLVM type information back in symbol table
        symbolTable.setLLVMType(name, typeStr);
        
        return llvmType;
    }
    
    // Fallback to old inference method
    return llvm::Type::getInt64Ty(context);
}

llvm::Type* IRCodegen::stringToLLVMType(const std::string& typeStr) {
    if (typeStr == "int") return llvm::Type::getInt64Ty(context);
    if (typeStr == "double") return llvm::Type::getDoubleTy(context);
    if (typeStr == "string") return llvm::PointerType::getUnqual(context);
    if (typeStr.find("void") != std::string::npos) return llvm::Type::getVoidTy(context);
    
    // Default fallback
    return llvm::Type::getInt64Ty(context);
}

// Legacy type inference method (kept for backward compatibility)
llvm::Type* IRCodegen::inferType(const Expr* expr) {
    if (auto lit = dynamic_cast<const LiteralExpr*>(expr)) {
        const std::string& v = lit->value;
        bool hasDot = false, allDigitsOrDot = !v.empty();
        for (char c : v) {
            if (c == '.') { hasDot = true; continue; }
            if (c < '0' || c > '9') { allDigitsOrDot = false; break; }
        }
        if (allDigitsOrDot && hasDot) return llvm::Type::getDoubleTy(context);
        if (allDigitsOrDot && !hasDot) return llvm::Type::getInt64Ty(context);
        return llvm::PointerType::getUnqual(context);
    }
    if (auto var = dynamic_cast<const VariableExpr*>(expr)) {
        // Use symbol table if possible, otherwise fallback
        return getTypeFromSymbolTable(var->name);
    }
    return llvm::Type::getInt64Ty(context);
}

// Builtin print overload resolution by argument type
llvm::Value* IRCodegen::ensurePrintCall(llvm::Value* arg) {
    if (!arg) return nullptr;  // Handle null arguments
    
    auto *ty = arg->getType();
    llvm::Function* callee = nullptr;

    if (ty->isIntegerTy(64)) {
        callee = getOrCreatePrintInt();
        return builder.CreateCall(callee, {arg});
    } else if (ty->isDoubleTy()) {
        callee = getOrCreatePrintDouble();
        return builder.CreateCall(callee, {arg});
    } else if (ty->isPointerTy()) {
        callee = getOrCreatePrintString();
        return builder.CreateCall(callee, {arg});
    } else {
        errorHandler.reportError(ErrorCategory::CODEGEN,
            "print: unsupported argument type in IR codegen",
            0, 0, "generating print call");
        return nullptr;
    }
}


// declare void @print_i64(i64)
llvm::Function* IRCodegen::getOrCreatePrintInt() {
    if (auto *F = module->getFunction("print_i64")) return F;
    auto *retTy = llvm::Type::getVoidTy(context);
    auto *i64   = llvm::Type::getInt64Ty(context);
    auto *fnTy  = llvm::FunctionType::get(retTy, {i64}, false);
    return llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage, "print_i64", module.get());
}

// declare void @print_double(double)
llvm::Function* IRCodegen::getOrCreatePrintDouble() {
    if (auto *F = module->getFunction("print_double")) return F;
    auto *retTy = llvm::Type::getVoidTy(context);
    auto *dbl   = llvm::Type::getDoubleTy(context);
    auto *fnTy  = llvm::FunctionType::get(retTy, {dbl}, false);
    return llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage, "print_double", module.get());
}

// declare void @print_str(ptr)
llvm::Function* IRCodegen::getOrCreatePrintString() {
    if (auto *F = module->getFunction("print_str")) return F;
    auto *retTy = llvm::Type::getVoidTy(context);
    auto *ptrTy = llvm::PointerType::getUnqual(context);
    auto *fnTy  = llvm::FunctionType::get(retTy, {ptrTy}, false);
    return llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage, "print_str", module.get());
}

// ===== Top-level emit =====

void IRCodegen::emit(const std::vector<std::unique_ptr<Stmt>>& statements) {
    auto *mainFn = createMain();

    // Emit each statement into main
    for (const auto& s : statements) {
        if (s) {  // Only emit non-null statements
            genStmt(s.get());
        }
    }

    // Return 0 from main
    builder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0));

    // Verify the module & main
    if (llvm::verifyFunction(*mainFn, &llvm::errs())) {
        errorHandler.reportError(ErrorCategory::CODEGEN,
            "IR verification failed for main()",
            0, 0, "verifying generated code");
        return;
    }
    if (llvm::verifyModule(*module, &llvm::errs())) {
        errorHandler.reportError(ErrorCategory::CODEGEN,
            "IR verification failed for module",
            0, 0, "verifying generated code");
        return;
    }
}

// ===== Statements =====

void IRCodegen::genStmt(const Stmt* stmt) {
    if (auto v = dynamic_cast<const VarDeclStmt*>(stmt)) {
        genVarDecl(v);
        return;
    }
    if (auto e = dynamic_cast<const ExprStmt*>(stmt)) {
        genExprStmt(e);
        return;
    }
    
    errorHandler.reportError(ErrorCategory::CODEGEN,
        "Unknown statement type in IR generation",
        0, 0, "generating statement");
}

void IRCodegen::genVarDecl(const VarDeclStmt* v) {
    // Phase 4: Use symbol table type information
    llvm::Type* ty = getTypeFromSymbolTable(v->name);

    auto *alloca = createEntryAlloca(currentFunction, ty, v->name);

    if (v->initializer) {
        llvm::Value* init = genExpr(v->initializer.get());
        if (!init) {
            // Expression generation failed, skip this variable
            return;
        }
        
        // Type checking using symbol table information
        if (init->getType() != ty) {
            errorHandler.reportError(ErrorCategory::CODEGEN,
                "Initializer type does not match declared variable type for: " + v->name,
                0, 0, "generating variable declaration");
            return;
        }
        
        builder.CreateStore(init, alloca);
    } else {
        // default-init based on type
        if (ty->isIntegerTy()) {
            builder.CreateStore(llvm::ConstantInt::get(ty, 0), alloca);
        } else if (ty->isDoubleTy()) {
            builder.CreateStore(llvm::ConstantFP::get(ty, 0.0), alloca);
        } else if (ty->isPointerTy()) {
            builder.CreateStore(llvm::ConstantPointerNull::get(static_cast<llvm::PointerType*>(ty)), alloca);
        }
    }

    namedValues[v->name] = alloca;
}

void IRCodegen::genExprStmt(const ExprStmt* s) {
    (void)genExpr(s->expression.get()); // ignore result if any
}

// ===== Expressions =====

llvm::Value* IRCodegen::genExpr(const Expr* expr) {
    if (!expr) return nullptr;  // Handle null expressions
    
    if (auto L = dynamic_cast<const LiteralExpr*>(expr))  return genLiteral(L);
    if (auto V = dynamic_cast<const VariableExpr*>(expr)) return genVariable(V);
    if (auto B = dynamic_cast<const BinaryExpr*>(expr))   return genBinary(B);
    if (auto C = dynamic_cast<const CallExpr*>(expr))     return genCall(C);
    
    errorHandler.reportError(ErrorCategory::CODEGEN,
        "Unknown expression type in IR generation",
        0, 0, "generating expression");
    return nullptr;
}

llvm::Value* IRCodegen::genLiteral(const LiteralExpr* lit) {
    const std::string& v = lit->value;

    // number?
    bool hasDot = false, allDigitsOrDot = !v.empty();
    for (char c : v) {
        if (c == '.') { hasDot = true; continue; }
        if (c < '0' || c > '9') { allDigitsOrDot = false; break; }
    }
    if (allDigitsOrDot && hasDot) {
        double d = std::strtod(v.c_str(), nullptr);
        return llvm::ConstantFP::get(llvm::Type::getDoubleTy(context), d);
    }
    if (allDigitsOrDot && !hasDot) {
        long long i = std::strtoll(v.c_str(), nullptr, 10);
        return llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), i);
    }

    // otherwise treat as string literal content -> ptr to a global
    return builder.CreateGlobalString(v, "str");
}

llvm::Value* IRCodegen::genVariable(const VariableExpr* var) {
    if (!symbolTable.isDeclared(var->name)) {
        errorHandler.reportError(ErrorCategory::CODEGEN,
            "Use of undeclared variable '" + var->name + "'",
            0, 0, "generating variable reference");
        return nullptr;
    }
    
    auto it = namedValues.find(var->name);
    if (it == namedValues.end()) {
        errorHandler.reportError(ErrorCategory::CODEGEN,
            "Variable not allocated: " + var->name,
            0, 0, "generating variable reference");
        return nullptr;
    }
    
    auto *alloca = it->second;
    llvm::Type* expectedType = getTypeFromSymbolTable(var->name);
    
    return builder.CreateLoad(expectedType, alloca, var->name + ".val");
}

llvm::Value* IRCodegen::genBinary(const BinaryExpr* /*bin*/) {
    errorHandler.reportError(ErrorCategory::CODEGEN,
        "Binary expressions are not supported in IR generation yet",
        0, 0, "generating binary expression");
    errorHandler.addSuggestion("Binary operations will be implemented in a future version");
    return nullptr;
}

llvm::Value* IRCodegen::genCall(const CallExpr* call) {
    if (call->callee == "print") {
        if (call->arguments.size() != 1) {
            errorHandler.reportError(ErrorCategory::CODEGEN,
                "print() expects exactly one argument, got " + std::to_string(call->arguments.size()),
                0, 0, "generating print call");
            return nullptr;
        }
        llvm::Value* argV = genExpr(call->arguments[0].get());
        if (!argV) return nullptr;  // Argument generation failed
        return ensurePrintCall(argV);
    }

    // User-defined functions
    llvm::Function* calleeFn = module->getFunction(call->callee);
    if (!calleeFn) {
        errorHandler.reportError(ErrorCategory::CODEGEN,
            "Call to unknown function: " + call->callee,
            0, 0, "generating function call");
        return nullptr;
    }

    std::vector<llvm::Value*> args;
    args.reserve(call->arguments.size());
    
    for (auto &a : call->arguments) {
        llvm::Value* arg = genExpr(a.get());
        if (!arg) return nullptr;  // Argument generation failed
        args.push_back(arg);
    }
    
    return builder.CreateCall(calleeFn, args);
}