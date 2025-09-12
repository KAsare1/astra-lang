#include "ir_codegen.h"
#include "../abstract-syntax-tree/ast.h"
#include "ir_codegen.h"
#include "../abstract-syntax-tree/ast.h"
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
    if (typeStr == "string") {
        // String as struct { i64 length, i8* data, i64 capacity }
        return getOrCreateStringType();
    }
    if (typeStr == "bool") return llvm::Type::getInt1Ty(context);
    if (typeStr.find("void") != std::string::npos) return llvm::Type::getVoidTy(context);
    
    // NEW: Handle array types
    if (isArrayTypeString(typeStr)) {
        std::string elementTypeStr = getArrayElementTypeString(typeStr);
        llvm::Type* elementType = stringToLLVMType(elementTypeStr);
        return getOrCreateArrayType(elementType);
    }
    
    return llvm::Type::getInt64Ty(context);  // Default fallback
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
    
    if (value->getType()->isIntegerTy(1)) {
        // Already a boolean (i1), use directly
        return value;
    } else if (value->getType()->isIntegerTy()) {
        // Convert integer to boolean
        llvm::Value* zero = llvm::ConstantInt::get(value->getType(), 0);
        return builder.CreateICmpNE(value, zero, "cond");
    } else if (value->getType()->isDoubleTy()) {
        // Convert double to boolean
        llvm::Value* zero = llvm::ConstantFP::get(value->getType(), 0.0);
        return builder.CreateFCmpONE(value, zero, "cond");
    } else {
        errorHandler.reportError(ErrorCategory::CODEGEN,
            "Cannot convert value to boolean condition", 0, 0, "generating condition");
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
    std::cout << "=== IR CODEGEN: Starting emission ===\n";
    
    // PASS 1: Declare all functions
    for (const auto& stmt : statements) {
        if (auto funcDecl = dynamic_cast<const FunctionDeclStmt*>(stmt.get())) {
            std::cout << "IR CODEGEN: Declaring function " << funcDecl->name << "\n";
            createFunction(funcDecl->name, funcDecl->parameters, funcDecl->returnType);
        }
    }
    
    // PASS 2: Generate function bodies
    for (const auto& stmt : statements) {
        if (auto funcDecl = dynamic_cast<const FunctionDeclStmt*>(stmt.get())) {
            std::cout << "IR CODEGEN: Generating function body for " << funcDecl->name << "\n";
            genFunctionDecl(funcDecl);
        }
    }
    
    // CREATE MAIN FOR TOP-LEVEL STATEMENTS - ADD THIS BACK
    auto *mainFn = createMain();
    currentFunctionReturnType = "int";
    
    // Generate main function body (non-function statements)
    for (const auto& stmt : statements) {
        if (!dynamic_cast<const FunctionDeclStmt*>(stmt.get()) && stmt) {
            genStmt(stmt.get());
        }
    }
    
    // Return 0 from main
    builder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0));  

    // Verify all functions
    for (const auto& [name, func] : functions) {
        if (llvm::verifyFunction(*func, &llvm::errs())) {
            errorHandler.reportError(ErrorCategory::CODEGEN,
                "IR verification failed for function: " + name, 0, 0, "verifying generated code");
        }
    }
    
    if (llvm::verifyModule(*module, &llvm::errs())) {
        errorHandler.reportError(ErrorCategory::CODEGEN,
            "IR verification failed for module", 0, 0, "verifying generated code");
    }
}
// ===== Statements =====

// 6. ADD genIndexAssignmentStmt() method (add to genStmt):
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
    // NEW: Handle index assignment
    if (auto indexAssign = dynamic_cast<const IndexAssignmentStmt*>(stmt)) {
        std::cout << "DEBUG: IR generating IndexAssignmentStmt\n";
        genIndexAssignmentStmt(indexAssign);
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
    if (auto forStmt = dynamic_cast<const ForStmt*>(stmt)) {
        std::cout << "DEBUG: IR generating ForStmt!\n";
        genForStmt(forStmt);
        return;
    }
    if (auto funcDecl = dynamic_cast<const FunctionDeclStmt*>(stmt)) {
        std::cout << "DEBUG: IR generating FunctionDeclStmt\n";
        genFunctionDecl(funcDecl);
        return;
    }
    if (auto retStmt = dynamic_cast<const ReturnStmt*>(stmt)) {
        std::cout << "DEBUG: IR generating ReturnStmt\n";
        genReturnStmt(retStmt);
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
        std::cerr << "[IR DEBUG] VarDecl: variable '" << v->name << "' LLVM type: ";
        ty->print(llvm::errs());
        std::cerr << std::endl;
        std::cerr << "[IR DEBUG] VarDecl: initializer LLVM type: ";
        init->getType()->print(llvm::errs());
        std::cerr << std::endl;
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
    if (!expr) return nullptr;
    
    if (auto L = dynamic_cast<const LiteralExpr*>(expr))    return genLiteral(L);
    if (auto V = dynamic_cast<const VariableExpr*>(expr))   return genVariable(V);
    if (auto U = dynamic_cast<const UnaryExpr*>(expr))      return genUnary(U);
    if (auto B = dynamic_cast<const BinaryExpr*>(expr))     return genBinary(B);
    if (auto C = dynamic_cast<const CallExpr*>(expr))       return genCall(C);
    if (auto R = dynamic_cast<const RangeExpr*>(expr))      return genRange(R);  // Existing
    
    // NEW: Handle data structure expressions
    if (auto arrayLit = dynamic_cast<const ArrayLiteralExpr*>(expr)) {
        return genArrayLiteral(arrayLit);
    }
    if (auto indexExpr = dynamic_cast<const IndexExpr*>(expr)) {
        return genIndexExpr(indexExpr);
    }
    
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
    
    // CHECK NAMEDVALUES FIRST (for function parameters and local variables)
    auto it = namedValues.find(var->name);
    if (it != namedValues.end()) {
        std::cout << "DEBUG: Variable found in namedValues, loading value" << std::endl;
        auto *alloca = it->second;
        
        // For parameters, try to infer type from the alloca
        llvm::Type* expectedType = alloca->getAllocatedType();
        return builder.CreateLoad(expectedType, alloca, var->name + ".val");
    }
    
    // THEN CHECK SYMBOL TABLE (for global variables)
    if (!symbolTable.isDeclared(var->name)) {
        std::cout << "DEBUG: Variable not found anywhere: " << var->name << std::endl;
        errorHandler.reportError(ErrorCategory::CODEGEN,
            "Use of undeclared variable '" + var->name + "'",
            0, 0, "generating variable reference");
        return nullptr;
    }
    
    std::cout << "DEBUG: Variable found in symbol table but not in namedValues" << std::endl;
    
    // This shouldn't happen for properly declared variables, but handle it
    llvm::Type* expectedType = getTypeFromSymbolTable(var->name);
    // Would need to create alloca here for global variables
    
    errorHandler.reportError(ErrorCategory::CODEGEN,
        "Variable not allocated: " + var->name,
        0, 0, "generating variable reference");
    return nullptr;
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

    if (bin->op == "+") {
        llvm::Type* leftType = left->getType();
        llvm::Type* rightType = right->getType();
        
        // Check if both are string structs
        if (leftType->isStructTy() && rightType->isStructTy()) {
            llvm::StructType* leftStruct = llvm::cast<llvm::StructType>(leftType);
            llvm::StructType* rightStruct = llvm::cast<llvm::StructType>(rightType);
            
            if (leftStruct->getName() == "string" && rightStruct->getName() == "string") {
                return createStringConcat(left, right);
            }
        }
        
        // Fall through to numeric addition
        if (left->getType() != right->getType()) {
            errorHandler.reportError(ErrorCategory::CODEGEN,
                "Type mismatch in binary expression",
                0, 0, "generating binary expression");
            return nullptr;
        }
        
        if (left->getType()->isIntegerTy()) {
            return builder.CreateAdd(left, right, "addtmp");
        } else if (left->getType()->isDoubleTy()) {
            return builder.CreateFAdd(left, right, "addtmp");
        }
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

        if (isBuiltinFunction(call->callee)) {
        return genBuiltinCall(call);
    }

    // User-defined functions
auto it = functions.find(call->callee);
if (it == functions.end()) {
    errorHandler.reportError(ErrorCategory::CODEGEN,
        "Call to unknown function: " + call->callee, 0, 0, "generating function call");
    return nullptr;
}
llvm::Function* calleeFn = it->second;

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



// NEW: Function creation and management methods
llvm::Function* IRCodegen::createFunction(const std::string& name, 
                                          const std::vector<Parameter>& params,
                                          const std::string& returnType) {
    if (functions.find(name) != functions.end()) {
        errorHandler.reportError(ErrorCategory::CODEGEN,
            "Function '" + name + "' already defined", 0, 0, "creating function");
        return nullptr;
    }
    
    llvm::FunctionType* funcType = createFunctionType(params, returnType);
    if (!funcType) return nullptr;
    
    llvm::Function* function = llvm::Function::Create(funcType, 
        llvm::Function::ExternalLinkage, name, module.get());
    
    auto paramIt = params.begin();
    for (auto& arg : function->args()) {
        if (paramIt != params.end()) {
            arg.setName(paramIt->name);
            ++paramIt;
        }
    }
    
    functions[name] = function;
    return function;
}

llvm::FunctionType* IRCodegen::createFunctionType(const std::vector<Parameter>& params,
                                                   const std::string& returnType) {
    llvm::Type* retType = stringToLLVMType(returnType);
    
    std::vector<llvm::Type*> paramTypes;
    for (const auto& param : params) {
        llvm::Type* paramType = stringToLLVMType(param.type);
        paramTypes.push_back(paramType);
    }
    
    return llvm::FunctionType::get(retType, paramTypes, false);
}

void IRCodegen::setupFunctionEntry(llvm::Function* function, const std::vector<Parameter>& params) {
    llvm::BasicBlock* entryBlock = llvm::BasicBlock::Create(context, "entry", function);
    builder.SetInsertPoint(entryBlock);
    
    std::cout << "DEBUG: Setting up parameters for function\n";
    
    auto paramIt = params.begin();
    for (auto& arg : function->args()) {
        if (paramIt != params.end()) {
            std::cout << "DEBUG: Adding parameter " << paramIt->name << " to namedValues\n";
            llvm::Type* paramType = stringToLLVMType(paramIt->type);
            llvm::AllocaInst* alloca = createEntryAlloca(function, paramType, paramIt->name);
            builder.CreateStore(&arg, alloca);
            namedValues[paramIt->name] = alloca;
            ++paramIt;
        }
    }
    
    std::cout << "DEBUG: namedValues after parameter setup:\n";
    for (const auto& pair : namedValues) {
        std::cout << "  - " << pair.first << std::endl;
    }
}

void IRCodegen::genFunctionDecl(const FunctionDeclStmt* funcDecl) {
    auto it = functions.find(funcDecl->name);
    if (it == functions.end()) {
        errorHandler.reportError(ErrorCategory::CODEGEN,
            "Function '" + funcDecl->name + "' not found in function table", 0, 0, "generating function");
        return;
    }
    
    llvm::Function* function = it->second;
    
    // Save current state
    llvm::Function* savedFunction = currentFunction;
    std::string savedReturnType = currentFunctionReturnType;
    auto savedNamedValues = namedValues;
    
    // Set up new function context
    currentFunction = function;
    currentFunctionReturnType = funcDecl->returnType;
    namedValues.clear();
    
    setupFunctionEntry(function, funcDecl->parameters);
    
    // Generate function body
    if (funcDecl->body) {
        for (const auto& stmt : funcDecl->body->statements) {
            if (stmt) {
                genStmt(stmt.get());
            }
        }
    }
    
    // Add default return if needed
    if (!builder.GetInsertBlock()->getTerminator()) {
        if (funcDecl->returnType == "void") {
            builder.CreateRetVoid();
        } else {
            llvm::Type* retType = stringToLLVMType(funcDecl->returnType);
            if (retType->isIntegerTy()) {
                builder.CreateRet(llvm::ConstantInt::get(retType, 0));
            } else if (retType->isDoubleTy()) {
                builder.CreateRet(llvm::ConstantFP::get(retType, 0.0));
            }
        }
    }
    
    // Restore previous state
    currentFunction = savedFunction;
    currentFunctionReturnType = savedReturnType;
    namedValues = savedNamedValues;
    
    if (savedFunction) {
        for (auto& block : *savedFunction) {
            if (!block.getTerminator()) {
                builder.SetInsertPoint(&block);
                break;
            }
        }
    }
}

void IRCodegen::genReturnStmt(const ReturnStmt* retStmt) {
    if (retStmt->value) {
        llvm::Value* retVal = genExpr(retStmt->value.get());
        if (!retVal) {
            errorHandler.reportError(ErrorCategory::CODEGEN,
                "Failed to generate return value", 0, 0, "generating return statement");
            return;
        }
        builder.CreateRet(retVal);
    } else {
        builder.CreateRetVoid();
    }
}


llvm::StructType* IRCodegen::getOrCreateArrayType(llvm::Type* elementType) {
    std::string typeName = "array_" + std::string(elementType->getStructName());
    
    auto it = arrayTypes.find(typeName);
    if (it != arrayTypes.end()) {
        return it->second;
    }
    
    // Create array struct: { i64 length, i64 capacity, elementType* data }
    std::vector<llvm::Type*> arrayFields = {
        llvm::Type::getInt64Ty(context),    // length
        llvm::Type::getInt64Ty(context),    // capacity  
        llvm::PointerType::getUnqual(elementType)  // data pointer
    };
    
    llvm::StructType* arrayType = llvm::StructType::create(context, arrayFields, typeName);
    arrayTypes[typeName] = arrayType;
    return arrayType;
}

llvm::StructType* IRCodegen::getOrCreateStringType() {
    static llvm::StructType* stringType = nullptr;
    
    if (!stringType) {
        // String struct: { i64 length, i8* data, i64 capacity }
        std::vector<llvm::Type*> stringFields = {
            llvm::Type::getInt64Ty(context),    // length
            llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context)),  // data
            llvm::Type::getInt64Ty(context)     // capacity
        };
        stringType = llvm::StructType::create(context, stringFields, "string");
    }
    
    return stringType;
}



llvm::Value* IRCodegen::genArrayLiteral(const ArrayLiteralExpr* arrayLit) {
    if (arrayLit->elements.empty()) {
        // Empty array - create with default element type
        llvm::Type* elementType = llvm::Type::getInt64Ty(context);  // Default to int
        llvm::Value* size = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), 0);
        llvm::Value* arr = createArrayAllocation(elementType, size);
        std::cerr << "[IR DEBUG] genArrayLiteral: returned LLVM type: ";
        arr->getType()->print(llvm::errs());
        std::cerr << std::endl;
        return arr;
    }

    // Generate first element to determine type
    llvm::Value* firstElement = genExpr(arrayLit->elements[0].get());
    if (!firstElement) return nullptr;

    llvm::Type* elementType = firstElement->getType();
    llvm::Value* arraySize = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), arrayLit->elements.size());

    // Create array allocation
    llvm::Value* arrayPtr = createArrayAllocation(elementType, arraySize);
    if (!arrayPtr) return nullptr;

    // Store first element
    llvm::Value* zeroIndex = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), 0);
    createArrayAssignment(arrayPtr, zeroIndex, firstElement);

    // Generate and store remaining elements
    for (size_t i = 1; i < arrayLit->elements.size(); ++i) {
        llvm::Value* element = genExpr(arrayLit->elements[i].get());
        if (!element) continue;

        // Type checking should be done in semantic analysis
        llvm::Value* index = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), i);
        createArrayAssignment(arrayPtr, index, element);
    }

    std::cerr << "[IR DEBUG] genArrayLiteral: returned LLVM type: ";
    arrayPtr->getType()->print(llvm::errs());
    std::cerr << std::endl;
    return arrayPtr;
}

// 5. ADD genIndexExpr() method:
llvm::Value* IRCodegen::genIndexExpr(const IndexExpr* indexExpr) {
    llvm::Value* object = genExpr(indexExpr->object.get());
    llvm::Value* index = genExpr(indexExpr->index.get());
    
    if (!object || !index) return nullptr;
    
    // Determine if this is array or string indexing
    llvm::Type* objectType = object->getType();
    
    if (objectType->isStructTy()) {
        llvm::StructType* structType = llvm::cast<llvm::StructType>(objectType);
        
        // Check if this is an array struct
        if (structType->getName().starts_with("array_")) {
            // Array indexing with bounds checking
            createBoundsCheck(object, index, "array access");
            return createArrayAccess(object, index);
        }
        // Check if this is a string struct
        else if (structType->getName() == "string") {
            // String indexing with bounds checking
            createBoundsCheck(object, index, "string access");
            return createStringAccess(object, index);
        }
    }
    
    errorHandler.reportError(ErrorCategory::CODEGEN,
        "Invalid indexing operation on non-indexable type",
        0, 0, "generating index expression");
    return nullptr;
}


void IRCodegen::genIndexAssignmentStmt(const IndexAssignmentStmt* indexAssign) {
    llvm::Value* object = genExpr(indexAssign->object.get());
    llvm::Value* index = genExpr(indexAssign->index.get());
    llvm::Value* value = genExpr(indexAssign->value.get());
    
    if (!object || !index || !value) {
        errorHandler.reportError(ErrorCategory::CODEGEN,
            "Failed to generate components of index assignment",
            0, 0, "generating index assignment");
        return;
    }
    
    // Add bounds checking
    createBoundsCheck(object, index, "array assignment");
    
    // Determine object type and perform assignment
    llvm::Type* objectType = object->getType();
    
    if (objectType->isStructTy()) {
        llvm::StructType* structType = llvm::cast<llvm::StructType>(objectType);
        
        if (structType->getName().starts_with("array_")) {
            createArrayAssignment(object, index, value);
        } else if (structType->getName() == "string") {
            createStringAssignment(object, index, value);
        } else {
            errorHandler.reportError(ErrorCategory::CODEGEN,
                "Invalid assignment to non-indexable type",
                0, 0, "generating index assignment");
        }
    } else {
        errorHandler.reportError(ErrorCategory::CODEGEN,
            "Invalid assignment to non-structured type",
            0, 0, "generating index assignment");
    }
}




// ===========================================
// ARRAY AND STRING HELPER IMPLEMENTATIONS
// ===========================================

// 8. ADD createArrayAllocation() method:
llvm::Value* IRCodegen::createArrayAllocation(llvm::Type* elementType, llvm::Value* size) {
    // Get array struct type
    llvm::StructType* arrayType = getOrCreateArrayType(elementType);
    
    // Allocate array struct on stack
    llvm::AllocaInst* arrayAlloca = createEntryAlloca(currentFunction, arrayType, "array");
    
    // Initialize array fields
    // Set length
    llvm::Value* lengthPtr = builder.CreateStructGEP(arrayType, arrayAlloca, 0, "length_ptr");
    builder.CreateStore(size, lengthPtr);
    
    // Set initial capacity (same as length for now)
    llvm::Value* capacityPtr = builder.CreateStructGEP(arrayType, arrayAlloca, 1, "capacity_ptr");
    builder.CreateStore(size, capacityPtr);
    
    // Allocate data array on heap
    llvm::Function* mallocFn = getOrCreateMalloc();
    llvm::Value* elementSize = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), 
                                                     module->getDataLayout().getTypeAllocSize(elementType));
    llvm::Value* totalSize = builder.CreateMul(size, elementSize, "total_size");
    llvm::Value* dataPtr = builder.CreateCall(mallocFn, {totalSize}, "data_ptr");
    
    // Cast to correct pointer type
    llvm::Value* typedDataPtr = builder.CreateBitCast(dataPtr, 
                                                     llvm::PointerType::getUnqual(elementType), 
                                                     "typed_data_ptr");
    
    // Store data pointer
    llvm::Value* dataPtrField = builder.CreateStructGEP(arrayType, arrayAlloca, 2, "data_ptr_field");
    builder.CreateStore(typedDataPtr, dataPtrField);
    
    return arrayAlloca;
}

// 9. ADD createArrayAccess() method:
llvm::Value* IRCodegen::createArrayAccess(llvm::Value* arrayPtr, llvm::Value* index) {
    // arrayPtr is an AllocaInst pointing to the array struct
    llvm::Type* arrayPtrType = arrayPtr->getType();
    
    // For AllocaInst, get the allocated type
    llvm::Type* arrayType = nullptr;
    if (auto* allocaInst = llvm::dyn_cast<llvm::AllocaInst>(arrayPtr)) {
        arrayType = allocaInst->getAllocatedType();
    } else if (arrayPtrType->isPointerTy()) {
        arrayType = arrayPtrType->getPointerElementType();
    } else {
        return nullptr; // Error case
    }
    
    if (!arrayType->isStructTy()) return nullptr;
    
    llvm::StructType* arrayStructType = llvm::cast<llvm::StructType>(arrayType);
    
    // Get data pointer from array struct (field 2)
    llvm::Value* dataPtrField = builder.CreateStructGEP(arrayStructType, arrayPtr, 2, "data_ptr_field");
    
    // Load the data pointer
    llvm::Type* dataPtrType = arrayStructType->getElementType(2); // Should be elementType*
    llvm::Value* dataPtr = builder.CreateLoad(dataPtrType, dataPtrField, "data_ptr");
    
    // Get element type from the data pointer type
    llvm::Type* elementType = dataPtrType->getPointerElementType();
    
    // Calculate element address
    llvm::Value* elementPtr = builder.CreateGEP(elementType, dataPtr, index, "element_ptr");
    
    // Load element
    return builder.CreateLoad(elementType, elementPtr, "element");
}
// 10. ADD createArrayAssignment() method:
llvm::Value* IRCodegen::createArrayAssignment(llvm::Value* arrayPtr, llvm::Value* index, llvm::Value* value) {
    // arrayPtr is an AllocaInst pointing to the array struct
    llvm::Type* arrayType = nullptr;
    if (auto* allocaInst = llvm::dyn_cast<llvm::AllocaInst>(arrayPtr)) {
        arrayType = allocaInst->getAllocatedType();
    } else if (arrayPtr->getType()->isPointerTy()) {
        arrayType = arrayPtr->getType()->getPointerElementType();
    } else {
        return nullptr;
    }
    
    if (!arrayType->isStructTy()) return nullptr;
    
    llvm::StructType* arrayStructType = llvm::cast<llvm::StructType>(arrayType);
    
    // Get data pointer from array struct (field 2)
    llvm::Value* dataPtrField = builder.CreateStructGEP(arrayStructType, arrayPtr, 2, "data_ptr_field");
    
    // Load the data pointer
    llvm::Type* dataPtrType = arrayStructType->getElementType(2);
    llvm::Value* dataPtr = builder.CreateLoad(dataPtrType, dataPtrField, "data_ptr");
    
    // Get element type
    llvm::Type* elementType = dataPtrType->getPointerElementType();
    
    // Calculate element address
    llvm::Value* elementPtr = builder.CreateGEP(elementType, dataPtr, index, "element_ptr");
    
    // Store value
    builder.CreateStore(value, elementPtr);
    return value;
}

// 11. ADD createStringLiteral() method:
llvm::Value* IRCodegen::createStringLiteral(const std::string& str) {
    llvm::StructType* stringType = getOrCreateStringType();
    
    // Allocate string struct
    llvm::AllocaInst* stringAlloca = createEntryAlloca(currentFunction, stringType, "string_lit");
    
    // Create global string constant
    llvm::Constant* globalStr = builder.CreateGlobalString(str, "str_literal");
    
    // Set length
    llvm::Value* lengthPtr = builder.CreateStructGEP(stringType, stringAlloca, 0, "length_ptr");
    llvm::Value* length = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), str.length());
    builder.CreateStore(length, lengthPtr);
    
    // Set data pointer
    llvm::Value* dataPtrField = builder.CreateStructGEP(stringType, stringAlloca, 1, "data_ptr_field");
    builder.CreateStore(globalStr, dataPtrField);
    
    // Set capacity (same as length for string literals)
    llvm::Value* capacityPtr = builder.CreateStructGEP(stringType, stringAlloca, 2, "capacity_ptr");
    builder.CreateStore(length, capacityPtr);
    
    return stringAlloca;
}

// 12. ADD createStringConcat() method:
llvm::Value* IRCodegen::createStringConcat(llvm::Value* left, llvm::Value* right) {
    llvm::Function* concatFn = getOrCreateStringConcatFunction();
    return builder.CreateCall(concatFn, {left, right}, "concat_result");
}

// 13. ADD createStringAccess() method:
llvm::Value* IRCodegen::createStringAccess(llvm::Value* stringPtr, llvm::Value* index) {
    llvm::StructType* stringType = getOrCreateStringType();
    
    // Get data pointer
    llvm::Value* dataPtrField = builder.CreateStructGEP(stringType, stringPtr, 1, "data_ptr_field");
    llvm::Value* dataPtr = builder.CreateLoad(dataPtrField->getType()->getArrayElementType(), 
                                            dataPtrField, "data_ptr");
    
    // Get character at index
    llvm::Value* charPtr = builder.CreateGEP(llvm::Type::getInt8Ty(context), dataPtr, index, "char_ptr");
    llvm::Value* charValue = builder.CreateLoad(llvm::Type::getInt8Ty(context), charPtr, "char");
    
    // Convert character to string (create single-character string)
    return createSingleCharString(charValue);
}

// 14. ADD createBoundsCheck() method:
void IRCodegen::createBoundsCheck(llvm::Value* object, llvm::Value* index, const std::string& context) {
    // Get the allocated type from the object
    llvm::Type* objectType = nullptr;
    if (auto* allocaInst = llvm::dyn_cast<llvm::AllocaInst>(object)) {
        objectType = allocaInst->getAllocatedType();
    } else if (object->getType()->isPointerTy()) {
        objectType = object->getType()->getPointerElementType();
    } else {
        return; // Can't bounds check this
    }
    
    if (!objectType->isStructTy()) return;
    
    llvm::StructType* structType = llvm::cast<llvm::StructType>(objectType);
    
    // Get length field (first field in both array and string structs)
    llvm::Value* lengthPtr = builder.CreateStructGEP(structType, object, 0, "length_ptr");
    llvm::Value* length = builder.CreateLoad(llvm::Type::getInt64Ty(context), lengthPtr, "length");
    
    // Check if index < 0
    llvm::Value* zero = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), 0);
    llvm::Value* indexNegative = builder.CreateICmpSLT(index, zero, "index_negative");
    
    // Check if index >= length
    llvm::Value* indexTooLarge = builder.CreateICmpSGE(index, length, "index_too_large");
    
    // Combine conditions: index < 0 || index >= length
    llvm::Value* outOfBounds = builder.CreateOr(indexNegative, indexTooLarge, "out_of_bounds");
    
    // Create conditional branch
    llvm::BasicBlock* errorBB = llvm::BasicBlock::Create(context, "bounds_error", currentFunction);
    llvm::BasicBlock* continueBB = llvm::BasicBlock::Create(context, "bounds_ok", currentFunction);
    
    builder.CreateCondBr(outOfBounds, errorBB, continueBB);
    
    // Error block - call bounds check function
    builder.SetInsertPoint(errorBB);
    llvm::Function* boundsCheckFn = getOrCreateBoundsCheckFunction();
    builder.CreateCall(boundsCheckFn, {index, length});
    llvm::Function* exitFn = getOrCreateErrorExit();
    llvm::Value* exitCode = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 1);
    builder.CreateCall(exitFn, {exitCode});
    builder.CreateUnreachable();
    
    // Continue with normal execution
    builder.SetInsertPoint(continueBB);
}



// 16. ADD genBuiltinCall() method:
llvm::Value* IRCodegen::genBuiltinCall(const CallExpr* call) {
    const std::string& funcName = call->callee;
    
    if (funcName == "len") {
        if (call->arguments.size() != 1) return nullptr;
        llvm::Value* arg = genExpr(call->arguments[0].get());
        if (!arg) return nullptr;
        
        // Get the allocated type for the argument
        llvm::Type* argType = nullptr;
        if (auto* allocaInst = llvm::dyn_cast<llvm::AllocaInst>(arg)) {
            argType = allocaInst->getAllocatedType();
        } else if (arg->getType()->isPointerTy()) {
            argType = arg->getType()->getPointerElementType();
        } else {
            return nullptr;
        }
        
        std::cerr << "[IR DEBUG] len() argument allocated type: ";
        argType->print(llvm::errs());
        std::cerr << std::endl;
        
        if (argType->isStructTy()) {
            llvm::StructType* structType = llvm::cast<llvm::StructType>(argType);
            if (structType->getName().starts_with("array_") || structType->getName() == "string") {
                llvm::Value* lengthPtr = builder.CreateStructGEP(structType, arg, 0, "length_ptr");
                return builder.CreateLoad(llvm::Type::getInt64Ty(context), lengthPtr, "length");
            }
        }
        return nullptr;
    }

    else if (funcName == "push") {
        if (call->arguments.size() != 2) return nullptr;
        
        llvm::Value* arrayArg = genExpr(call->arguments[0].get());
        llvm::Value* elementArg = genExpr(call->arguments[1].get());
        
        if (!arrayArg || !elementArg) return nullptr;
        
        llvm::Function* pushFn = getOrCreateArrayPushFunction();
        return builder.CreateCall(pushFn, {arrayArg, elementArg});
    }
    else if (funcName == "pop") {
        if (call->arguments.size() != 1) return nullptr;
        
        llvm::Value* arrayArg = genExpr(call->arguments[0].get());
        if (!arrayArg) return nullptr;
        
        llvm::Function* popFn = getOrCreateArrayPopFunction();
        return builder.CreateCall(popFn, {arrayArg});
    }
    else if (funcName == "concat") {
        if (call->arguments.size() != 2) return nullptr;
        
        llvm::Value* leftArg = genExpr(call->arguments[0].get());
        llvm::Value* rightArg = genExpr(call->arguments[1].get());
        
        if (!leftArg || !rightArg) return nullptr;
        
        return createStringConcat(leftArg, rightArg);
    }
    else if (funcName == "substr") {
        if (call->arguments.size() != 3) return nullptr;
        
        llvm::Value* stringArg = genExpr(call->arguments[0].get());
        llvm::Value* startArg = genExpr(call->arguments[1].get());
        llvm::Value* lenArg = genExpr(call->arguments[2].get());
        
        if (!stringArg || !startArg || !lenArg) return nullptr;
        
        llvm::Function* substrFn = getOrCreateStringSubstrFunction();
        return builder.CreateCall(substrFn, {stringArg, startArg, lenArg});
    }
    else if (funcName == "input") {
        if (call->arguments.size() != 0) return nullptr;
        
        llvm::Function* inputFn = getOrCreateInputFunction();
        return builder.CreateCall(inputFn, {});
    }
    else if (funcName == "println") {
        if (call->arguments.size() != 1) return nullptr;
        
        llvm::Value* arg = genExpr(call->arguments[0].get());
        if (!arg) return nullptr;
        
        llvm::Function* printlnFn = getOrCreatePrintlnFunction();
        return builder.CreateCall(printlnFn, {arg});
    }
    else if (funcName == "to_string") {
        if (call->arguments.size() != 1) return nullptr;
        
        llvm::Value* arg = genExpr(call->arguments[0].get());
        if (!arg) return nullptr;
        
        llvm::Function* toStringFn = getOrCreateToStringFunction();
        return builder.CreateCall(toStringFn, {arg});
    }
    else if (funcName == "to_int") {
        if (call->arguments.size() != 1) return nullptr;
        
        llvm::Value* arg = genExpr(call->arguments[0].get());
        if (!arg) return nullptr;
        
        llvm::Function* toIntFn = getOrCreateToIntFunction();
        return builder.CreateCall(toIntFn, {arg});
    }
    else if (funcName == "to_double") {
        if (call->arguments.size() != 1) return nullptr;
        
        llvm::Value* arg = genExpr(call->arguments[0].get());
        if (!arg) return nullptr;
        
        llvm::Function* toDoubleFn = getOrCreateToDoubleFunction();
        return builder.CreateCall(toDoubleFn, {arg});
    }
    else if (funcName == "print") {
        // Existing print implementation
        if (call->arguments.size() != 1) return nullptr;
        
        llvm::Value* argV = genExpr(call->arguments[0].get());
        if (!argV) return nullptr;
        
        return ensurePrintCall(argV);
    }
    
    return nullptr;  // Unknown built-in function
}

// 17. ADD helper to check if function is built-in:
bool IRCodegen::isBuiltinFunction(const std::string& name) {
    return name == "len" || name == "push" || name == "pop" || 
           name == "concat" || name == "substr" || name == "input" || 
           name == "println" || name == "to_string" || name == "to_int" || 
           name == "to_double" || name == "print";
}

// ===========================================
// RUNTIME FUNCTION DECLARATIONS
// ===========================================

// 18. ADD getOrCreateMalloc() method:
llvm::Function* IRCodegen::getOrCreateMalloc() {
    if (auto *F = module->getFunction("malloc")) return F;
    
    auto *i8PtrTy = llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context));
    auto *sizeTy = llvm::Type::getInt64Ty(context);
    auto *fnTy = llvm::FunctionType::get(i8PtrTy, {sizeTy}, false);
    
    return llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage, "malloc", module.get());
}

// 19. ADD getOrCreateFree() method:
llvm::Function* IRCodegen::getOrCreateFree() {
    if (auto *F = module->getFunction("free")) return F;
    
    auto *voidTy = llvm::Type::getVoidTy(context);
    auto *i8PtrTy = llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context));
    auto *fnTy = llvm::FunctionType::get(voidTy, {i8PtrTy}, false);
    
    return llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage, "free", module.get());
}

// 20. ADD getOrCreateArrayPushFunction() method:
llvm::Function* IRCodegen::getOrCreateArrayPushFunction() {
    if (auto *F = module->getFunction("array_push")) return F;
    
    auto *voidTy = llvm::Type::getVoidTy(context);
    auto *i8PtrTy = llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context));
    auto *fnTy = llvm::FunctionType::get(voidTy, {i8PtrTy, i8PtrTy}, false);
    
    return llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage, "array_push", module.get());
}

// 21. ADD getOrCreateArrayPopFunction() method:
llvm::Function* IRCodegen::getOrCreateArrayPopFunction() {
    if (auto *F = module->getFunction("array_pop")) return F;
    
    auto *i8PtrTy = llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context));
    auto *fnTy = llvm::FunctionType::get(i8PtrTy, {i8PtrTy}, false);
    
    return llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage, "array_pop", module.get());
}

// 22. ADD getOrCreateStringConcatFunction() method:
llvm::Function* IRCodegen::getOrCreateStringConcatFunction() {
    if (auto *F = module->getFunction("string_concat")) return F;
    
    llvm::StructType* stringType = getOrCreateStringType();
    auto *stringPtrTy = llvm::PointerType::getUnqual(stringType);
    auto *fnTy = llvm::FunctionType::get(stringPtrTy, {stringPtrTy, stringPtrTy}, false);
    
    return llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage, "string_concat", module.get());
}

// 23. ADD getOrCreateStringSubstrFunction() method:
llvm::Function* IRCodegen::getOrCreateStringSubstrFunction() {
    if (auto *F = module->getFunction("string_substr")) return F;
    
    llvm::StructType* stringType = getOrCreateStringType();
    auto *stringPtrTy = llvm::PointerType::getUnqual(stringType);
    auto *i64Ty = llvm::Type::getInt64Ty(context);
    auto *fnTy = llvm::FunctionType::get(stringPtrTy, {stringPtrTy, i64Ty, i64Ty}, false);
    
    return llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage, "string_substr", module.get());
}

// 24. ADD getOrCreateInputFunction() method:
llvm::Function* IRCodegen::getOrCreateInputFunction() {
    if (auto *F = module->getFunction("input_string")) return F;
    
    llvm::StructType* stringType = getOrCreateStringType();
    auto *stringPtrTy = llvm::PointerType::getUnqual(stringType);
    auto *fnTy = llvm::FunctionType::get(stringPtrTy, {}, false);
    
    return llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage, "input_string", module.get());
}

// 25. ADD getOrCreatePrintlnFunction() method:
llvm::Function* IRCodegen::getOrCreatePrintlnFunction() {
    if (auto *F = module->getFunction("println_any")) return F;
    
    auto *voidTy = llvm::Type::getVoidTy(context);
    auto *i8PtrTy = llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context));
    auto *fnTy = llvm::FunctionType::get(voidTy, {i8PtrTy}, false);
    
    return llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage, "println_any", module.get());
}

// 26. ADD getOrCreateToStringFunction() method:
llvm::Function* IRCodegen::getOrCreateToStringFunction() {
    if (auto *F = module->getFunction("to_string_any")) return F;
    
    llvm::StructType* stringType = getOrCreateStringType();
    auto *stringPtrTy = llvm::PointerType::getUnqual(stringType);
    auto *i8PtrTy = llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context));
    auto *fnTy = llvm::FunctionType::get(stringPtrTy, {i8PtrTy}, false);
    
    return llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage, "to_string_any", module.get());
}

// 27. ADD getOrCreateToIntFunction() method:
llvm::Function* IRCodegen::getOrCreateToIntFunction() {
    if (auto *F = module->getFunction("string_to_int")) return F;
    
    auto *i64Ty = llvm::Type::getInt64Ty(context);
    llvm::StructType* stringType = getOrCreateStringType();
    auto *stringPtrTy = llvm::PointerType::getUnqual(stringType);
    auto *fnTy = llvm::FunctionType::get(i64Ty, {stringPtrTy}, false);
    
    return llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage, "string_to_int", module.get());
}

// 28. ADD getOrCreateToDoubleFunction() method:
llvm::Function* IRCodegen::getOrCreateToDoubleFunction() {
    if (auto *F = module->getFunction("string_to_double")) return F;
    
    auto *doubleTy = llvm::Type::getDoubleTy(context);
    llvm::StructType* stringType = getOrCreateStringType();
    auto *stringPtrTy = llvm::PointerType::getUnqual(stringType);
    auto *fnTy = llvm::FunctionType::get(doubleTy, {stringPtrTy}, false);
    
    return llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage, "string_to_double", module.get());
}

// 29. ADD getOrCreateBoundsCheckFunction() method:
llvm::Function* IRCodegen::getOrCreateBoundsCheckFunction() {
    if (auto *F = module->getFunction("bounds_check_error")) return F;
    
    auto *voidTy = llvm::Type::getVoidTy(context);
    auto *i64Ty = llvm::Type::getInt64Ty(context);
    auto *fnTy = llvm::FunctionType::get(voidTy, {i64Ty, i64Ty}, false);
    
    return llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage, "bounds_check_error", module.get());
}

// 30. ADD getOrCreateErrorExit() method:
llvm::Function* IRCodegen::getOrCreateErrorExit() {
    if (auto *F = module->getFunction("exit")) return F;
    
    auto *voidTy = llvm::Type::getVoidTy(context);
    auto *i32Ty = llvm::Type::getInt32Ty(context);
    auto *fnTy = llvm::FunctionType::get(voidTy, {i32Ty}, false);
    
    return llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage, "exit", module.get());
}

// ===========================================
// TYPE HELPER IMPLEMENTATIONS
// ===========================================

// 31. ADD isArrayTypeString() method:
bool IRCodegen::isArrayTypeString(const std::string& typeStr) {
    return typeStr.size() >= 3 && typeStr[0] == '[' && typeStr.back() == ']';
}

// 32. ADD getArrayElementTypeString() method:
std::string IRCodegen::getArrayElementTypeString(const std::string& arrayTypeStr) {
    if (!isArrayTypeString(arrayTypeStr)) return arrayTypeStr;
    
    // "[int]" -> "int", "[[string]]" -> "[string]"
    return arrayTypeStr.substr(1, arrayTypeStr.size() - 2);
}

// 33. ADD createSingleCharString() method:
llvm::Value* IRCodegen::createSingleCharString(llvm::Value* charValue) {
    llvm::StructType* stringType = getOrCreateStringType();
    
    // Allocate string struct
    llvm::AllocaInst* stringAlloca = createEntryAlloca(currentFunction, stringType, "char_string");
    
    // Allocate single character on heap
    llvm::Function* mallocFn = getOrCreateMalloc();
    llvm::Value* one = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), 1);
    llvm::Value* charPtr = builder.CreateCall(mallocFn, {one}, "char_ptr");
    
    // Store character
    builder.CreateStore(charValue, charPtr);
    
    // Set string fields
    llvm::Value* lengthPtr = builder.CreateStructGEP(stringType, stringAlloca, 0, "length_ptr");
    builder.CreateStore(one, lengthPtr);
    
    llvm::Value* dataPtrField = builder.CreateStructGEP(stringType, stringAlloca, 1, "data_ptr_field");
    builder.CreateStore(charPtr, dataPtrField);
    
    llvm::Value* capacityPtr = builder.CreateStructGEP(stringType, stringAlloca, 2, "capacity_ptr");
    builder.CreateStore(one, capacityPtr);
    
    return stringAlloca;
}

// 34. ADD createStringAssignment() method:
llvm::Value* IRCodegen::createStringAssignment(llvm::Value* stringPtr, llvm::Value* index, llvm::Value* value) {
    llvm::StructType* stringType = getOrCreateStringType();
    
    // Get data pointer
    llvm::Value* dataPtrField = builder.CreateStructGEP(stringType, stringPtr, 1, "data_ptr_field");
    llvm::Value* dataPtr = builder.CreateLoad(dataPtrField->getType()->getArrayElementType(), 
                                            dataPtrField, "data_ptr");
    
    // Get character pointer at index
    llvm::Value* charPtr = builder.CreateGEP(llvm::Type::getInt8Ty(context), dataPtr, index, "char_ptr");
    
    // Convert value to character if it's a string
    llvm::Value* charToStore = value;
    if (value->getType()->isStructTy()) {
        llvm::StructType* valueType = llvm::cast<llvm::StructType>(value->getType()->getArrayElementType());
        if (valueType->getName() == "string") {
            // Extract first character from string
            llvm::Value* valueDataPtr = builder.CreateStructGEP(valueType, value, 1, "value_data_ptr");
            llvm::Value* valueData = builder.CreateLoad(valueDataPtr->getType()->getArrayElementType(), 
                                                       valueDataPtr, "value_data");
            llvm::Value* zero = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), 0);
            llvm::Value* firstCharPtr = builder.CreateGEP(llvm::Type::getInt8Ty(context), valueData, zero, "first_char_ptr");
            charToStore = builder.CreateLoad(llvm::Type::getInt8Ty(context), firstCharPtr, "first_char");
        }
    }
    
    // Store character
    builder.CreateStore(charToStore, charPtr);
    return value;
}

// Generate an LLVM struct representing a range (start, end, step)
llvm::Value* IRCodegen::genRange(const RangeExpr* rangeExpr) {
    // Generate start, end, and step values
    llvm::Value* startV = genExpr(rangeExpr->start.get());
    llvm::Value* endV = genExpr(rangeExpr->end.get());
    llvm::Value* stepV = nullptr;
    if (rangeExpr->step) {
        stepV = genExpr(rangeExpr->step.get());
    } else {
        stepV = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), 1);
    }

    // Create an LLVM struct { start, end, step }
    std::vector<llvm::Type*> types = {
        llvm::Type::getInt64Ty(context),
        llvm::Type::getInt64Ty(context),
        llvm::Type::getInt64Ty(context)
    };
    llvm::StructType* rangeTy = llvm::StructType::get(context, types);
    llvm::Value* rangeAlloca = builder.CreateAlloca(rangeTy, nullptr, "range");
    llvm::Value* startPtr = builder.CreateStructGEP(rangeTy, rangeAlloca, 0, "start_ptr");
    llvm::Value* endPtr = builder.CreateStructGEP(rangeTy, rangeAlloca, 1, "end_ptr");
    llvm::Value* stepPtr = builder.CreateStructGEP(rangeTy, rangeAlloca, 2, "step_ptr");
    builder.CreateStore(startV, startPtr);
    builder.CreateStore(endV, endPtr);
    builder.CreateStore(stepV, stepPtr);
    return rangeAlloca;
}


llvm::Type* IRCodegen::getAllocatedType(llvm::Value* value) {
    if (auto* allocaInst = llvm::dyn_cast<llvm::AllocaInst>(value)) {
        return allocaInst->getAllocatedType();
    } else if (value->getType()->isPointerTy()) {
        return value->getType()->getPointerElementType();
    }
    return nullptr;
}