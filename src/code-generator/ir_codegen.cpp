#include "ir_codegen.h"
#include <stdexcept>
#include <cstdlib>
#include "../shared/error_handler.h"
#include "llvm/IR/Value.h"

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

// NEW: Helper to convert values to boolean for conditions
llvm::Value* IRCodegen::createCondition(llvm::Value* value) {
    if (!value) return nullptr;
    
    if (value->getType()->isIntegerTy()) {
        // For integers, compare with zero
        llvm::Value* zero = llvm::ConstantInt::get(value->getType(), 0);
        return builder.CreateICmpNE(value, zero, "cond");
    } else if (value->getType()->isDoubleTy()) {
        // For doubles, compare with 0.0
        llvm::Value* zero = llvm::ConstantFP::get(value->getType(), 0.0);
        return builder.CreateFCmpONE(value, zero, "cond");
    } else {
        errorHandler.reportError(ErrorCategory::CODEGEN,
            "Cannot convert value to boolean condition",
            0, 0, "generating condition");
        return nullptr;
    }
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
    std::cout << "DEBUG: IR genStmt called\n";
    
    if (auto v = dynamic_cast<const VarDeclStmt*>(stmt)) {
        std::cout << "DEBUG: IR generating VarDeclStmt\n";
        genVarDecl(v);
        return;
    }
    if (auto e = dynamic_cast<const ExprStmt*>(stmt)) {
        std::cout << "DEBUG: IR generating ExprStmt\n";
        genExprStmt(e);
        return;
    }
    if (auto assign = dynamic_cast<const AssignmentStmt*>(stmt)) {
        std::cout << "DEBUG: IR generating AssignmentStmt\n";
        genAssignmentStmt(assign);
        return;
    }
    if (auto block = dynamic_cast<const BlockStmt*>(stmt)) {
        std::cout << "DEBUG: IR generating BlockStmt\n";
        genBlockStmt(block);
        return;
    }
    if (auto ifStmt = dynamic_cast<const IfStmt*>(stmt)) {
        std::cout << "DEBUG: IR generating IfStmt\n";
        genIfStmt(ifStmt);
        return;
    }
    if (auto whileStmt = dynamic_cast<const WhileStmt*>(stmt)) {
        std::cout << "DEBUG: IR generating WhileStmt\n";
        genWhileStmt(whileStmt);
        return;
    }
    // ADD THIS CHECK
    if (auto forStmt = dynamic_cast<const ForStmt*>(stmt)) {
        std::cout << "DEBUG: IR generating ForStmt!\n";
        genForStmt(forStmt);
        return;
    }
    
    std::cout << "DEBUG: IR unknown statement type!\n";
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

// NEW: Generate code for block statements
void IRCodegen::genBlockStmt(const BlockStmt* block) {
    // Save current named values for scope restoration
    auto savedValues = namedValues;
    
    // Generate code for all statements in the block
    for (const auto& stmt : block->statements) {
        if (stmt) {
            genStmt(stmt.get());
        }
    }
    
    // Restore previous scope (remove block-local variables)
    namedValues = savedValues;
}

// NEW: Generate code for if statements
void IRCodegen::genIfStmt(const IfStmt* ifStmt) {
    // Generate condition
    llvm::Value* condV = genExpr(ifStmt->condition.get());
    if (!condV) return;
    
    // Convert condition to boolean
    condV = createCondition(condV);
    if (!condV) return;
    
    // Create basic blocks
    llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(context, "then", currentFunction);
    llvm::BasicBlock* elseBB = nullptr;
    llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(context, "ifcont", currentFunction);
    
    if (ifStmt->elseBranch) {
        elseBB = llvm::BasicBlock::Create(context, "else", currentFunction);
        builder.CreateCondBr(condV, thenBB, elseBB);
    } else {
        builder.CreateCondBr(condV, thenBB, mergeBB);
    }
    
    // Generate then branch
    builder.SetInsertPoint(thenBB);
    genStmt(ifStmt->thenBranch.get());
    
    // Branch to merge block if no terminator was generated
    if (!builder.GetInsertBlock()->getTerminator()) {
        builder.CreateBr(mergeBB);
    }
    
    // Generate else branch if present
    if (ifStmt->elseBranch) {
        builder.SetInsertPoint(elseBB);
        genStmt(ifStmt->elseBranch.get());
        
        // Branch to merge block if no terminator was generated
        if (!builder.GetInsertBlock()->getTerminator()) {
            builder.CreateBr(mergeBB);
        }
    }
    
    // Continue with merge block
    builder.SetInsertPoint(mergeBB);
}

// NEW: Generate code for while statements
void IRCodegen::genWhileStmt(const WhileStmt* whileStmt) {
    // Create basic blocks
    llvm::BasicBlock* condBB = llvm::BasicBlock::Create(context, "whilecond", currentFunction);
    llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(context, "whilebody", currentFunction);
    llvm::BasicBlock* afterBB = llvm::BasicBlock::Create(context, "afterwhile", currentFunction);
    
    // Branch to condition block
    builder.CreateBr(condBB);
    
    // Generate condition block
    builder.SetInsertPoint(condBB);
    llvm::Value* condV = genExpr(whileStmt->condition.get());
    if (!condV) return;
    
    // Convert condition to boolean
    condV = createCondition(condV);
    if (!condV) return;
    
    builder.CreateCondBr(condV, bodyBB, afterBB);
    
    // Generate body block
    builder.SetInsertPoint(bodyBB);
    genStmt(whileStmt->body.get());
    
    // Branch back to condition if no terminator was generated
    if (!builder.GetInsertBlock()->getTerminator()) {
        builder.CreateBr(condBB);
    }
    
    // Continue with after block
    builder.SetInsertPoint(afterBB);
}

// ===== Expressions =====

llvm::Value* IRCodegen::genExpr(const Expr* expr) {
    if (!expr) return nullptr;  // Handle null expressions
    
    if (auto L = dynamic_cast<const LiteralExpr*>(expr))  return genLiteral(L);
    if (auto V = dynamic_cast<const VariableExpr*>(expr)) return genVariable(V);
    if (auto U = dynamic_cast<const UnaryExpr*>(expr))    return genUnary(U);
    if (auto B = dynamic_cast<const BinaryExpr*>(expr))   return genBinary(B);
    if (auto C = dynamic_cast<const CallExpr*>(expr))     return genCall(C);
    
    errorHandler.reportError(ErrorCategory::CODEGEN,
        "Unknown expression type in IR generation",
        0, 0, "generating expression");
    return nullptr;
}

llvm::Value* IRCodegen::genLiteral(const LiteralExpr* lit) {
    const std::string& v = lit->value;

    // Handle boolean literals
    if (v == "1" || v == "0") {
        long long i = std::strtoll(v.c_str(), nullptr, 10);
        return llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), i);
    }

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
    std::cout << "DEBUG: genVariable called for: " << var->name << std::endl;
    
    if (!symbolTable.isDeclared(var->name)) {
        std::cout << "DEBUG: Variable not declared in symbol table: " << var->name << std::endl;
        errorHandler.reportError(ErrorCategory::CODEGEN,
            "Use of undeclared variable '" + var->name + "'",
            0, 0, "generating variable reference");
        return nullptr;
    }
    
    auto it = namedValues.find(var->name);
    if (it == namedValues.end()) {
        std::cout << "DEBUG: Variable not found in namedValues: " << var->name << std::endl;
        std::cout << "DEBUG: Available variables in namedValues:" << std::endl;
        for (const auto& pair : namedValues) {
            std::cout << "  - " << pair.first << std::endl;
        }
        errorHandler.reportError(ErrorCategory::CODEGEN,
            "Variable not allocated: " + var->name,
            0, 0, "generating variable reference");
        return nullptr;
    }
    
    std::cout << "DEBUG: Variable found, loading value" << std::endl;
    
    auto *alloca = it->second;
    llvm::Type* expectedType = getTypeFromSymbolTable(var->name);
    
    return builder.CreateLoad(expectedType, alloca, var->name + ".val");
}

llvm::Value* IRCodegen::genUnary(const UnaryExpr* unary) {
    llvm::Value* operand = genExpr(unary->operand.get());
    if (!operand) {
        errorHandler.reportError(ErrorCategory::CODEGEN,
            "Failed to generate operand for unary expression",
            0, 0, "generating unary expression");
        return nullptr;
    }

    if (unary->op == "-") {
        if (operand->getType()->isIntegerTy()) {
            return builder.CreateNeg(operand, "negtmp");
        } else if (operand->getType()->isDoubleTy()) {
            return builder.CreateFNeg(operand, "negtmp");
        } else {
            errorHandler.reportError(ErrorCategory::CODEGEN,
                "Unsupported type for unary minus operator",
                0, 0, "generating unary expression");
            return nullptr;
        }
    } else if (unary->op == "!") {
        if (operand->getType()->isIntegerTy()) {
            // Convert to boolean, then negate
            llvm::Value* zero = llvm::ConstantInt::get(operand->getType(), 0);
            llvm::Value* cmp = builder.CreateICmpEQ(operand, zero, "cmptmp");
            return builder.CreateZExt(cmp, llvm::Type::getInt64Ty(context), "booltmp");
        } else {
            errorHandler.reportError(ErrorCategory::CODEGEN,
                "Logical NOT operator requires integer type",
                0, 0, "generating unary expression");
            return nullptr;
        }
    } else {
        errorHandler.reportError(ErrorCategory::CODEGEN,
            "Unsupported unary operator: " + unary->op,
            0, 0, "generating unary expression");
        return nullptr;
    }
}

llvm::Value* IRCodegen::genBinary(const BinaryExpr* bin) {
    llvm::Value* left = genExpr(bin->left.get());
    llvm::Value* right = genExpr(bin->right.get());

    if (!left || !right) {
        errorHandler.reportError(ErrorCategory::CODEGEN,
            "Null operand in binary expression",
            0, 0, "generating binary expression");
        return nullptr;
    }

    // Type checking - both operands should have the same type
    if (left->getType() != right->getType()) {
        errorHandler.reportError(ErrorCategory::CODEGEN,
            "Type mismatch in binary expression",
            0, 0, "generating binary expression");
        return nullptr;
    }

    // Arithmetic operations
    if (bin->op == "+") {
        if (left->getType()->isIntegerTy()) {
            return builder.CreateAdd(left, right, "addtmp");
        } else if (left->getType()->isDoubleTy()) {
            return builder.CreateFAdd(left, right, "addtmp");
        }
    } else if (bin->op == "-") {
        if (left->getType()->isIntegerTy()) {
            return builder.CreateSub(left, right, "subtmp");
        } else if (left->getType()->isDoubleTy()) {
            return builder.CreateFSub(left, right, "subtmp");
        }
    } else if (bin->op == "*") {
        if (left->getType()->isIntegerTy()) {
            return builder.CreateMul(left, right, "multmp");
        } else if (left->getType()->isDoubleTy()) {
            return builder.CreateFMul(left, right, "multmp");
        }
    } else if (bin->op == "/") {
        if (left->getType()->isIntegerTy()) {
            return builder.CreateSDiv(left, right, "divtmp");
        } else if (left->getType()->isDoubleTy()) {
            return builder.CreateFDiv(left, right, "divtmp");
        }
    } else if (bin->op == "%") {
        if (left->getType()->isIntegerTy()) {
            return builder.CreateSRem(left, right, "modtmp");
        } else {
            errorHandler.reportError(ErrorCategory::CODEGEN,
                "Modulo operator not supported for floating point types",
                0, 0, "generating binary expression");
            return nullptr;
        }
    }
    
    // Comparison operations
    else if (bin->op == "==") {
        if (left->getType()->isIntegerTy()) {
            llvm::Value* cmp = builder.CreateICmpEQ(left, right, "eqtmp");
            return builder.CreateZExt(cmp, llvm::Type::getInt64Ty(context), "booltmp");
        } else if (left->getType()->isDoubleTy()) {
            llvm::Value* cmp = builder.CreateFCmpOEQ(left, right, "eqtmp");
            return builder.CreateZExt(cmp, llvm::Type::getInt64Ty(context), "booltmp");
        }
    } else if (bin->op == "!=") {
        if (left->getType()->isIntegerTy()) {
            llvm::Value* cmp = builder.CreateICmpNE(left, right, "netmp");
            return builder.CreateZExt(cmp, llvm::Type::getInt64Ty(context), "booltmp");
        } else if (left->getType()->isDoubleTy()) {
            llvm::Value* cmp = builder.CreateFCmpONE(left, right, "netmp");
            return builder.CreateZExt(cmp, llvm::Type::getInt64Ty(context), "booltmp");
        }
    } else if (bin->op == "<") {
        if (left->getType()->isIntegerTy()) {
            llvm::Value* cmp = builder.CreateICmpSLT(left, right, "lttmp");
            return builder.CreateZExt(cmp, llvm::Type::getInt64Ty(context), "booltmp");
        } else if (left->getType()->isDoubleTy()) {
            llvm::Value* cmp = builder.CreateFCmpOLT(left, right, "lttmp");
            return builder.CreateZExt(cmp, llvm::Type::getInt64Ty(context), "booltmp");
        }
    } else if (bin->op == "<=") {
        if (left->getType()->isIntegerTy()) {
            llvm::Value* cmp = builder.CreateICmpSLE(left, right, "letmp");
            return builder.CreateZExt(cmp, llvm::Type::getInt64Ty(context), "booltmp");
        } else if (left->getType()->isDoubleTy()) {
            llvm::Value* cmp = builder.CreateFCmpOLE(left, right, "letmp");
            return builder.CreateZExt(cmp, llvm::Type::getInt64Ty(context), "booltmp");
        }
    } else if (bin->op == ">") {
        if (left->getType()->isIntegerTy()) {
            llvm::Value* cmp = builder.CreateICmpSGT(left, right, "gttmp");
            return builder.CreateZExt(cmp, llvm::Type::getInt64Ty(context), "booltmp");
        } else if (left->getType()->isDoubleTy()) {
            llvm::Value* cmp = builder.CreateFCmpOGT(left, right, "gttmp");
            return builder.CreateZExt(cmp, llvm::Type::getInt64Ty(context), "booltmp");
        }
    } else if (bin->op == ">=") {
        if (left->getType()->isIntegerTy()) {
            llvm::Value* cmp = builder.CreateICmpSGE(left, right, "getmp");
            return builder.CreateZExt(cmp, llvm::Type::getInt64Ty(context), "booltmp");
        } else if (left->getType()->isDoubleTy()) {
            llvm::Value* cmp = builder.CreateFCmpOGE(left, right, "getmp");
            return builder.CreateZExt(cmp, llvm::Type::getInt64Ty(context), "booltmp");
        }
    }
    
    // Logical operations
    else if (bin->op == "&&") {
        if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
            // Convert operands to boolean values first
            llvm::Value* leftBool = builder.CreateICmpNE(left, 
                llvm::ConstantInt::get(left->getType(), 0), "leftbool");
            llvm::Value* rightBool = builder.CreateICmpNE(right, 
                llvm::ConstantInt::get(right->getType(), 0), "rightbool");
            
            llvm::Value* result = builder.CreateAnd(leftBool, rightBool, "andtmp");
            return builder.CreateZExt(result, llvm::Type::getInt64Ty(context), "booltmp");
        }
    } else if (bin->op == "||") {
        if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
            // Convert operands to boolean values first
            llvm::Value* leftBool = builder.CreateICmpNE(left, 
                llvm::ConstantInt::get(left->getType(), 0), "leftbool");
            llvm::Value* rightBool = builder.CreateICmpNE(right, 
                llvm::ConstantInt::get(right->getType(), 0), "rightbool");
            
            llvm::Value* result = builder.CreateOr(leftBool, rightBool, "ortmp");
            return builder.CreateZExt(result, llvm::Type::getInt64Ty(context), "booltmp");
        }
    }

    errorHandler.reportError(ErrorCategory::CODEGEN,
        "Unsupported binary operator '" + bin->op + "' or unsupported type combination",
        0, 0, "generating binary expression");
    return nullptr;
}

llvm::Value* IRCodegen::genCall(const CallExpr* call) {
    if (call->callee == "print") {
        std::cout << "DEBUG: Processing print function call" << std::endl;
        if (call->arguments.size() != 1) {
            errorHandler.reportError(ErrorCategory::CODEGEN,
                "print() expects exactly one argument, got " + std::to_string(call->arguments.size()),
                0, 0, "generating print call");
            return nullptr;
        }
        std::cout << "DEBUG: Generating argument for print" << std::endl;
        llvm::Value* argV = genExpr(call->arguments[0].get());
        if (!argV) {
            std::cout << "DEBUG: Failed to generate argument for print" << std::endl;
            return nullptr;
        }
        std::cout << "DEBUG: Calling ensurePrintCall" << std::endl;
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



void IRCodegen::genAssignmentStmt(const AssignmentStmt* assignStmt) {
    // Check if variable exists in our named values (should have been declared)
    auto it = namedValues.find(assignStmt->name);
    if (it == namedValues.end()) {
        errorHandler.reportError(ErrorCategory::CODEGEN,
            "Assignment to undeclared variable: " + assignStmt->name,
            0, 0, "generating assignment");
        return;
    }
    
    // Generate the value expression
    llvm::Value* value = genExpr(assignStmt->value.get());
    if (!value) {
        errorHandler.reportError(ErrorCategory::CODEGEN,
            "Failed to generate value for assignment to: " + assignStmt->name,
            0, 0, "generating assignment");
        return;
    }
    
    // Get the variable's alloca instruction
    llvm::AllocaInst* alloca = it->second;
    
    // Type check - ensure the value type matches the variable type
    llvm::Type* expectedType = getTypeFromSymbolTable(assignStmt->name);
    if (value->getType() != expectedType) {
        errorHandler.reportError(ErrorCategory::CODEGEN,
            "Type mismatch in assignment to variable: " + assignStmt->name,
            0, 0, "generating assignment");
        return;
    }
    
    // Store the value into the variable
    builder.CreateStore(value, alloca);
}


// Enhanced genForStmt with step support
void IRCodegen::genForStmt(const ForStmt* forStmt) {
    // Extract range information
    auto rangeExpr = dynamic_cast<const RangeExpr*>(forStmt->range.get());
    if (!rangeExpr) {
        errorHandler.reportError(ErrorCategory::CODEGEN,
            "For loop range must be a range expression",
            0, 0, "generating for statement");
        return;
    }
    
    // Generate start, end, and step values
    llvm::Value* startV = genExpr(rangeExpr->start.get());
    llvm::Value* endV = genExpr(rangeExpr->end.get());
    
    // NEW: Generate step value (default to 1 if not provided)
    llvm::Value* stepV;
    if (rangeExpr->step) {
        stepV = genExpr(rangeExpr->step.get());
        if (!stepV) {
            errorHandler.reportError(ErrorCategory::CODEGEN,
                "Failed to generate step value for for loop",
                0, 0, "generating for statement");
            return;
        }
    } else {
        // Default step is 1
        stepV = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), 1);
    }
    
    if (!startV || !endV) {
        errorHandler.reportError(ErrorCategory::CODEGEN,
            "Failed to generate range bounds for for loop",
            0, 0, "generating for statement");
        return;
    }
    
    // Create alloca for loop variable
    llvm::Type* i64Ty = llvm::Type::getInt64Ty(context);
    auto* loopVarAlloca = createEntryAlloca(currentFunction, i64Ty, forStmt->variable);
    namedValues[forStmt->variable] = loopVarAlloca;
    
    // Initialize loop variable with start value
    builder.CreateStore(startV, loopVarAlloca);
    
    // Create basic blocks
    llvm::BasicBlock* condBB = llvm::BasicBlock::Create(context, "forcond", currentFunction);
    llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(context, "forbody", currentFunction);
    llvm::BasicBlock* afterBB = llvm::BasicBlock::Create(context, "afterfor", currentFunction);
    
    // Branch to condition block
    builder.CreateBr(condBB);
    
    // Generate condition block - handle both positive and negative steps
    builder.SetInsertPoint(condBB);
    llvm::Value* currentVal = builder.CreateLoad(i64Ty, loopVarAlloca, forStmt->variable + ".val");
    
    // NEW: Step-aware condition
    // If step > 0: currentVal <= endV
    // If step < 0: currentVal >= endV
    // If step == 0: error (should be caught in semantic analysis)
    
    // Check if step is positive or negative at runtime
    llvm::Value* zero = llvm::ConstantInt::get(i64Ty, 0);
    llvm::Value* stepIsPositive = builder.CreateICmpSGT(stepV, zero, "step_is_positive");
    
    // Create conditions for both cases
    llvm::Value* condPositive = builder.CreateICmpSLE(currentVal, endV, "cond_positive");  // i <= end
    llvm::Value* condNegative = builder.CreateICmpSGE(currentVal, endV, "cond_negative");  // i >= end
    
    // Select the appropriate condition based on step sign
    llvm::Value* condV = builder.CreateSelect(stepIsPositive, condPositive, condNegative, "forcond");
    
    builder.CreateCondBr(condV, bodyBB, afterBB);
    
    // Generate body block
    builder.SetInsertPoint(bodyBB);
    if (forStmt->body) {
        genStmt(forStmt->body.get());
    }
    
    // NEW: Step-based increment: i = i + step
    llvm::Value* bodyCurrentVal = builder.CreateLoad(i64Ty, loopVarAlloca, forStmt->variable + ".bodyval");
    llvm::Value* nextVal = builder.CreateAdd(bodyCurrentVal, stepV, "nextval");
    builder.CreateStore(nextVal, loopVarAlloca);
    
    // Branch back to condition if no terminator was generated
    if (!builder.GetInsertBlock()->getTerminator()) {
        builder.CreateBr(condBB);
    }
    
    // Continue with after block
    builder.SetInsertPoint(afterBB);
    
    // Clean up loop variable
    namedValues.erase(forStmt->variable);
}