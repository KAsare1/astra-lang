#include "ir_codegen.h"
#include <stdexcept>
#include <cstdlib>

// ===== IRCodegen =====

IRCodegen::IRCodegen(const std::string& moduleName, SymbolTable& syms)
    : module(std::make_unique<llvm::Module>(moduleName, context)),
      builder(context) {}

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

// Very small heuristic typing for now:
//  - all digits -> i64
//  - digits with '.' -> double
//  - else -> ptr (string)
// You can upgrade this to use your SymbolTable types later.
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
        // treat everything else as string literal (opaque pointer)
        return llvm::PointerType::getUnqual(context);
    }
    if (auto var = dynamic_cast<const VariableExpr*>(expr)) {
        // If you set symbol types, you could switch here.
        // For now default to i64 and let CallExpr(print) handle casts if needed.
        (void)var;
        return llvm::Type::getInt64Ty(context);
    }
    // Default fallback
    return llvm::Type::getInt64Ty(context);
}

// Builtin print overload resolution by argument type
llvm::Value* IRCodegen::ensurePrintCall(llvm::Value* arg) {
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
        // Try simple promotions: i32 -> i64, float -> double, etc. (not implemented here)
        throw std::runtime_error("print: unsupported argument type in IR codegen");
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
        genStmt(s.get());
    }

    // Return 0 from main
    builder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0));

    // Verify the module & main
    if (llvm::verifyFunction(*mainFn, &llvm::errs())) {
        throw std::runtime_error("IR verification failed for main()");
    }
    if (llvm::verifyModule(*module, &llvm::errs())) {
        throw std::runtime_error("IR verification failed for module");
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
    throw std::runtime_error("Unknown statement in IR codegen");
}

void IRCodegen::genVarDecl(const VarDeclStmt* v) {
    // Infer type from initializer or default to i64
    llvm::Type* ty = llvm::Type::getInt64Ty(context);
    if (v->initializer) {
        ty = inferType(v->initializer.get());
    }

    // allocate in entry block
    auto *alloca = createEntryAlloca(currentFunction, ty, v->name);

    // store initializer if present
    if (v->initializer) {
        llvm::Value* init = genExpr(v->initializer.get());
        // Simple match: if mismatched (e.g., const string vs i64), you'd insert casts here
        if (init->getType() != ty) {
            throw std::runtime_error("Initializer type does not match declared variable storage (prototype stage).");
        }
        builder.CreateStore(init, alloca);
    } else {
        // default-init to 0 / null
        if (ty->isIntegerTy()) {
            builder.CreateStore(llvm::ConstantInt::get(ty, 0), alloca);
        } else if (ty->isDoubleTy()) {
            builder.CreateStore(llvm::ConstantFP::get(ty, 0.0), alloca);
        } else if (ty->isPointerTy()) {
            builder.CreateStore(llvm::ConstantPointerNull::get(static_cast<llvm::PointerType*>(ty)), alloca);
        }
    }

    namedValues[v->name] = alloca;  // register local
    // (Optional) symbols.setType(v->name, ...string name for type...)
}

void IRCodegen::genExprStmt(const ExprStmt* s) {
    (void)genExpr(s->expression.get()); // ignore result if any
}

// ===== Expressions =====

llvm::Value* IRCodegen::genExpr(const Expr* expr) {
    if (auto L = dynamic_cast<const LiteralExpr*>(expr))  return genLiteral(L);
    if (auto V = dynamic_cast<const VariableExpr*>(expr)) return genVariable(V);
    if (auto B = dynamic_cast<const BinaryExpr*>(expr))   return genBinary(B);
    if (auto C = dynamic_cast<const CallExpr*>(expr))     return genCall(C);
    throw std::runtime_error("Unknown expression in IR codegen");
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
    auto it = namedValues.find(var->name);
    if (it == namedValues.end()) {
        throw std::runtime_error("IR: use of undeclared variable '" + var->name + "'");
    }
    auto *alloca = it->second;
    return builder.CreateLoad(alloca->getAllocatedType(), alloca, var->name + ".val");
}

llvm::Value* IRCodegen::genBinary(const BinaryExpr* /*bin*/) {
    // Not implemented yet (your language doesn't parse binary ops yet).
    throw std::runtime_error("Binary expressions are not supported in IR generation (yet).");
}

llvm::Value* IRCodegen::genCall(const CallExpr* call) {
    // For now, support 'print' as a builtin with simple overloads
    if (call->callee == "print") {
        if (call->arguments.size() != 1) {
            throw std::runtime_error("print() expects exactly one argument.");
        }
        llvm::Value* argV = genExpr(call->arguments[0].get());
        return ensurePrintCall(argV); // returns void (Value* is CallInst), ignored by ExprStmt
    }

    // If you later add user functions, look them up:
    llvm::Function* calleeFn = module->getFunction(call->callee);
    if (!calleeFn) {
        throw std::runtime_error("Call to unknown function: " + call->callee);
    }

    std::vector<llvm::Value*> args;
    args.reserve(call->arguments.size());
    for (auto &a : call->arguments) {
        args.push_back(genExpr(a.get()));
    }
    return builder.CreateCall(calleeFn, args);
}