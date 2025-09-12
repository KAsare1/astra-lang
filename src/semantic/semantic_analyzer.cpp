#include "semantic_analyzer.h"
#include <iostream>

SemanticAnalyzer::SemanticAnalyzer(SymbolTable& symbolTable, ErrorHandler& errors) 
    : symbols(symbolTable), errorHandler(errors), currentLine(0),
      currentFunctionName(""), currentFunctionReturnType(""), inFunctionBody(false) {
    initializeBuiltins();
}

void SemanticAnalyzer::analyze(const std::vector<std::unique_ptr<Stmt>>& statements) {
    for (const auto& stmt : statements) {
        if (stmt) {  // Only analyze non-null statements
            currentLine++;  // Simple line tracking
            analyzeStmt(stmt.get());
        }
    }
    
    // Phase 3: Check for unused variables (as warnings)
    checkForUnusedVariables();
}

const SymbolTable& SemanticAnalyzer::getSymbolTable() const {
    return symbols;
}

void SemanticAnalyzer::initializeBuiltins() {
    // Existing built-ins
    symbols.declare("print", SymbolKind::FUNCTION);
    symbols.setType("print", "void(any)");
    symbols.markUsed("print");
    symbols.markBuiltin("print");
    
    // NEW: Generic built-ins
    symbols.declare("len", SymbolKind::FUNCTION);
    symbols.setType("len", "int(any)");  // Generic: works with arrays and strings
    symbols.markBuiltin("len");
    
    symbols.declare("println", SymbolKind::FUNCTION);
    symbols.setType("println", "void(any)");
    symbols.markBuiltin("println");
    
    // NEW: Array-specific built-ins
    symbols.declare("push", SymbolKind::FUNCTION);
    symbols.setType("push", "void([any],any)");  // push(array, element)
    symbols.markBuiltin("push");
    
    symbols.declare("pop", SymbolKind::FUNCTION);
    symbols.setType("pop", "any([any])");  // pop(array) -> element
    symbols.markBuiltin("pop");
    
    // NEW: String built-ins
    symbols.declare("concat", SymbolKind::FUNCTION);
    symbols.setType("concat", "string(string,string)");
    symbols.markBuiltin("concat");
    
    symbols.declare("substr", SymbolKind::FUNCTION);
    symbols.setType("substr", "string(string,int,int)");  // substr(str, start, len)
    symbols.markBuiltin("substr");
    
    symbols.declare("input", SymbolKind::FUNCTION);
    symbols.setType("input", "string()");
    symbols.markBuiltin("input");
    
    // NEW: Type conversion built-ins
    symbols.declare("to_string", SymbolKind::FUNCTION);
    symbols.setType("to_string", "string(any)");
    symbols.markBuiltin("to_string");
    
    symbols.declare("to_int", SymbolKind::FUNCTION);
    symbols.setType("to_int", "int(string)");
    symbols.markBuiltin("to_int");
    
    symbols.declare("to_double", SymbolKind::FUNCTION);
    symbols.setType("to_double", "double(string)");
    symbols.markBuiltin("to_double");
}

void SemanticAnalyzer::analyzeStmt(const Stmt* stmt) {
    if (auto varDecl = dynamic_cast<const VarDeclStmt*>(stmt)) {
        handleVarDecl(varDecl);
    }
    else if (auto exprStmt = dynamic_cast<const ExprStmt*>(stmt)) {
        analyzeExpr(exprStmt->expression.get());
    }
    else if (auto assignStmt = dynamic_cast<const AssignmentStmt*>(stmt)) {
        handleAssignmentStmt(assignStmt);
    }
    // NEW: Handle index assignment
    else if (auto indexAssign = dynamic_cast<const IndexAssignmentStmt*>(stmt)) {
        handleIndexAssignmentStmt(indexAssign);
    }
    else if (auto blockStmt = dynamic_cast<const BlockStmt*>(stmt)) {
        handleBlockStmt(blockStmt);
    }
    else if (auto ifStmt = dynamic_cast<const IfStmt*>(stmt)) {
        handleIfStmt(ifStmt);
    }
    else if (auto whileStmt = dynamic_cast<const WhileStmt*>(stmt)) {
        handleWhileStmt(whileStmt);
    }
    else if (auto forStmt = dynamic_cast<const ForStmt*>(stmt)) {
        handleForStmt(forStmt);
    }
    else if (auto funcDecl = dynamic_cast<const FunctionDeclStmt*>(stmt)) {
        handleFunctionDecl(funcDecl);
    }
    else if (auto retStmt = dynamic_cast<const ReturnStmt*>(stmt)) {
        handleReturnStmt(retStmt);
    }
    else {
        errorHandler.reportError(ErrorCategory::SEMANTIC,
            "Unknown statement type in semantic analysis",
            currentLine, 0, "analyzing statement");
    }
}

void SemanticAnalyzer::handleVarDecl(const VarDeclStmt* varDecl) {
    // Check if already declared in current scope
    if (symbols.isDeclaredInCurrentScope(varDecl->name)) {
        errorHandler.reportError(ErrorCategory::SEMANTIC,
            "Variable '" + varDecl->name + "' is already declared in this scope",
            currentLine, 0, "declaring variable");
        errorHandler.addSuggestion("Use a different variable name");
        return;
    }
    
    // Declare the variable first
    symbols.declare(varDecl->name, SymbolKind::VARIABLE);
    
    std::string declaredType = "";
    
    // Handle explicit type annotation
    if (!varDecl->type.empty()) {
        declaredType = varDecl->type;
        validateTypeDeclaration(declaredType);
    }
    
    // Analyze initializer and infer/check type
    if (varDecl->initializer) {
        std::string initializerType = analyzeExpr(varDecl->initializer.get());
        if (initializerType == "error") {
            return;
        }
        
        if (declaredType.empty()) {
            // Type inference from initializer
            declaredType = initializerType;
        } else {
            // Type checking against declared type
            if (!areTypesCompatible(declaredType, initializerType)) {
                errorHandler.reportError(ErrorCategory::SEMANTIC,
                    "Initializer type '" + initializerType + 
                    "' does not match declared type '" + declaredType + 
                    "' for variable '" + varDecl->name + "'",
                    currentLine, 0, "declaring variable");
                errorHandler.addSuggestion("Ensure the initializer matches the declared type");
                return;
            }
        }
        
        symbols.setType(varDecl->name, declaredType);
        symbols.markInitialized(varDecl->name);
    } else {
        // No initializer - use declared type or default
        if (declaredType.empty()) {
            declaredType = "int";  // Default type
        }
        symbols.setType(varDecl->name, declaredType);
        
        // Arrays and complex types should be initialized
        if (isArrayType(declaredType)) {
            errorHandler.reportWarning(ErrorCategory::SEMANTIC,
                "Array variable '" + varDecl->name + "' declared without initialization",
                currentLine, 0, "declaring variable");
            errorHandler.addSuggestion("Initialize arrays with a literal like '[1, 2, 3]' or '[]'");
        }
        
        symbols.markInitialized(varDecl->name);  // Consider uninitialized vars as default-initialized
    }
}


// Handle assignment statements
void SemanticAnalyzer::handleAssignmentStmt(const AssignmentStmt* assignStmt) {
    // Check if variable is declared
    if (!symbols.isDeclared(assignStmt->name)) {
        errorHandler.reportError(ErrorCategory::SEMANTIC,
            "Assignment to undeclared variable '" + assignStmt->name + "'",
            currentLine, 0, "analyzing assignment");
        errorHandler.addSuggestion("Declare the variable with 'let " + assignStmt->name + " = ...' first");
        return;
    }
    
    // Get the variable's current type
    std::string varType = symbols.getType(assignStmt->name);
    
    // Analyze the value expression
    std::string valueType = analyzeExpr(assignStmt->value.get());
    
    if (valueType == "error") {
        return;
    }
    
    // Enhanced type compatibility checking for arrays
    if (!areTypesCompatible(varType, valueType)) {
        errorHandler.reportError(ErrorCategory::SEMANTIC,
            "Type mismatch in assignment: cannot assign '" + valueType + 
            "' to variable '" + assignStmt->name + "' of type '" + varType + "'",
            currentLine, 0, "analyzing assignment");
        
        // Provide specific suggestions for common array mistakes
        if (isArrayType(varType) && !isArrayType(valueType)) {
            errorHandler.addSuggestion("Use array literal syntax like '[1, 2, 3]' for array assignment");
        } else if (!isArrayType(varType) && isArrayType(valueType)) {
            errorHandler.addSuggestion("Declare variable as array type like 'let " + assignStmt->name + ": " + valueType + "'");
        } else {
            errorHandler.addSuggestion("Ensure the assigned value matches the variable's type");
        }
        return;
    }
    
    // Mark variable as used and initialized
    symbols.markUsed(assignStmt->name);
    symbols.markInitialized(assignStmt->name);
}

void SemanticAnalyzer::handleBlockStmt(const BlockStmt* blockStmt) {
    symbols.enterScope();  // Enter new scope for block
    
    for (const auto& stmt : blockStmt->statements) {
        if (stmt) {
            analyzeStmt(stmt.get());
        }
    }
    
    symbols.exitScope();  // Exit scope after block
}

void SemanticAnalyzer::handleIfStmt(const IfStmt* ifStmt) {
    std::string conditionType = analyzeExpr(ifStmt->condition.get());
    
    if (conditionType != "error") {
        // UPDATED: Accept bool, int, and double for conditions
        if (conditionType != "int" && conditionType != "double" && conditionType != "bool") {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "If condition must be boolean-convertible (numeric or bool), got '" + conditionType + "'",
                currentLine, 0, "analyzing if statement");
            errorHandler.addSuggestion("Use a comparison expression like 'x > 0' or a boolean variable");
        }
    }
    
    if (ifStmt->thenBranch) {
        analyzeStmt(ifStmt->thenBranch.get());
    }
    
    if (ifStmt->elseBranch) {
        analyzeStmt(ifStmt->elseBranch.get());
    }
}

void SemanticAnalyzer::handleWhileStmt(const WhileStmt* whileStmt) {
    std::string conditionType = analyzeExpr(whileStmt->condition.get());
    
    if (conditionType != "error") {
        // UPDATED: Accept bool, int, and double for conditions
        if (conditionType != "int" && conditionType != "double" && conditionType != "bool") {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "While condition must be boolean-convertible (numeric or bool), got '" + conditionType + "'",
                currentLine, 0, "analyzing while statement");
            errorHandler.addSuggestion("Use a comparison expression like 'i < 10' or a boolean variable");
        }
    }
    
    if (whileStmt->body) {
        analyzeStmt(whileStmt->body.get());
    }
}

// NEW: Handle for statements
void SemanticAnalyzer::handleForStmt(const ForStmt* forStmt) {
    std::cout << "DEBUG: handleForStmt called for variable: " << forStmt->variable << std::endl;
    
    // Analyze the range expression
    std::cout << "DEBUG: About to analyze range expression\n";
    std::string rangeType = analyzeExpr(forStmt->range.get());
    std::cout << "DEBUG: Range type returned: " << rangeType << std::endl;
    
    if (rangeType == "error") {
        std::cout << "DEBUG: Range analysis returned error, exiting\n";
        return;  // Error already reported
    }
    
    // Range should be a valid range expression
    if (rangeType != "range") {
        std::cout << "DEBUG: Range type is not 'range', got: " << rangeType << std::endl;
        errorHandler.reportError(ErrorCategory::SEMANTIC,
            "For loop requires a range expression, got '" + rangeType + "'",
            currentLine, 0, "analyzing for statement");
        errorHandler.addSuggestion("Use range syntax like 'start:end'");
        return;
    }
    
    std::cout << "DEBUG: Range validation passed, declaring loop variable in current scope\n";
    
    // FIXED: Declare the loop variable in the CURRENT scope, not a nested scope
    // Check if loop variable already exists in current scope
    if (symbols.isDeclaredInCurrentScope(forStmt->variable)) {
        errorHandler.reportError(ErrorCategory::SEMANTIC,
            "Loop variable '" + forStmt->variable + "' conflicts with existing variable",
            currentLine, 0, "analyzing for statement");
        return;
    }
    
    // Declare the loop variable as an integer in the current scope
    std::cout << "DEBUG: Declaring loop variable: " << forStmt->variable << std::endl;
    symbols.declare(forStmt->variable, SymbolKind::VARIABLE);
    symbols.setType(forStmt->variable, "int");
    symbols.markInitialized(forStmt->variable);
    symbols.markUsed(forStmt->variable); // Loop variables are inherently "used"
    
    std::cout << "DEBUG: Loop variable declared, analyzing body\n";
    
    // Analyze the loop body (no separate scope needed for semantic analysis)
    if (forStmt->body) {
        analyzeStmt(forStmt->body.get());
    }
    
    std::cout << "DEBUG: handleForStmt completed\n";
    
    // NOTE: We don't remove the loop variable from the symbol table here
    // because the IR generator will need to access it. In a real compiler,
    // we might want more sophisticated scope management.
}

std::string SemanticAnalyzer::analyzeExpr(const Expr* expr) {
    if (!expr) return "error";
    
    if (auto varExpr = dynamic_cast<const VariableExpr*>(expr)) {
        // Existing variable analysis...
        if (!symbols.isDeclared(varExpr->name)) {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "Use of undeclared variable '" + varExpr->name + "'",
                currentLine, 0, "analyzing expression");
            return "error";
        }
        
        const Symbol* symbol = symbols.getSymbol(varExpr->name);
        if (symbol && symbol->kind == SymbolKind::VARIABLE && !symbol->isInitialized) {
            errorHandler.reportWarning(ErrorCategory::SEMANTIC,
                "Variable '" + varExpr->name + "' may be used before initialization",
                currentLine, 0, "analyzing variable usage");
        }
        
        symbols.markUsed(varExpr->name);
        return symbols.getType(varExpr->name);
    }
    else if (auto literal = dynamic_cast<const LiteralExpr*>(expr)) {
        // Existing literal analysis...
        const std::string& v = literal->value;
        
        if (v == "0" || v == "1") return "int";
        
        bool hasDot = false, allDigitsOrDot = !v.empty();
        for (char c : v) {
            if (c == '.') { hasDot = true; continue; }
            if (c < '0' || c > '9') { allDigitsOrDot = false; break; }
        }
        if (allDigitsOrDot && hasDot) return "double";
        if (allDigitsOrDot && !hasDot) return "int";
        return "string";
    }
    // NEW: Handle array literals
    else if (auto arrayLit = dynamic_cast<const ArrayLiteralExpr*>(expr)) {
        return analyzeArrayLiteral(arrayLit);
    }
    // NEW: Handle indexing expressions
    else if (auto indexExpr = dynamic_cast<const IndexExpr*>(expr)) {
        return analyzeIndexExpr(indexExpr);
    }
    else if (auto unary = dynamic_cast<const UnaryExpr*>(expr)) {
        // Existing unary analysis...
        std::string operandType = analyzeExpr(unary->operand.get());
        if (operandType == "error") return "error";
        
        if (unary->op == "-") {
            if (operandType == "int" || operandType == "double") {
                return operandType;
            } else {
                errorHandler.reportError(ErrorCategory::SEMANTIC,
                    "Unary minus requires numeric type, got '" + operandType + "'",
                    currentLine, 0, "analyzing unary expression");
                return "error";
            }
        } else if (unary->op == "!") {
            return "bool";
        } else {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "Unknown unary operator: " + unary->op,
                currentLine, 0, "analyzing unary expression");
            return "error";
        }
    }
    else if (auto binary = dynamic_cast<const BinaryExpr*>(expr)) {
        // ENHANCED: Handle string concatenation and array operations
        std::string leftType = analyzeExpr(binary->left.get());
        std::string rightType = analyzeExpr(binary->right.get());
        
        if (leftType == "error" || rightType == "error") {
            return "error";
        }
        
        return inferBinaryResultType(leftType, rightType, binary->op);
    }
    else if (auto call = dynamic_cast<const CallExpr*>(expr)) {
        // ENHANCED: Handle built-in functions
        if (symbols.isFunction(call->callee) && 
            symbols.getSymbol(call->callee)->isBuiltin) {
            return analyzeBuiltinCall(call);
        }
        
        // Existing function call analysis...
        if (!symbols.isDeclared(call->callee)) {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "Call to undeclared function '" + call->callee + "'",
                currentLine, 0, "analyzing function call");
            return "error";
        }
        
        symbols.markUsed(call->callee);
        
        if (symbols.isFunction(call->callee)) {
            std::string returnType = symbols.getFunctionReturnType(call->callee);
            
            for (const auto& arg : call->arguments) {
                std::string argType = analyzeExpr(arg.get());
                if (argType == "error") return "error";
            }
            
            return returnType;
        }
        
        return symbols.getType(call->callee);
    }
    else if (auto rangeExpr = dynamic_cast<const RangeExpr*>(expr)) {
        // Existing range analysis...
        std::string startType = analyzeExpr(rangeExpr->start.get());
        std::string endType = analyzeExpr(rangeExpr->end.get());
        
        if (startType == "error" || endType == "error") {
            return "error";
        }
        
        if (startType != "int" || endType != "int") {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "Range expressions require integer bounds, got '" + 
                startType + "' and '" + endType + "'",
                currentLine, 0, "analyzing range expression");
            return "error";
        }
        
        if (rangeExpr->step) {
            std::string stepType = analyzeExpr(rangeExpr->step.get());
            if (stepType == "error") return "error";
            if (stepType != "int") {
                errorHandler.reportError(ErrorCategory::SEMANTIC,
                    "Range step must be an integer, got '" + stepType + "'",
                    currentLine, 0, "analyzing range expression");
                return "error";
            }
            
            if (auto stepLiteral = dynamic_cast<const LiteralExpr*>(rangeExpr->step.get())) {
                if (stepLiteral->value == "0") {
                    errorHandler.reportError(ErrorCategory::SEMANTIC,
                        "Range step cannot be zero",
                        currentLine, 0, "analyzing range expression");
                    return "error";
                }
            }
        }
        
        return "range";
    }
    
    errorHandler.reportError(ErrorCategory::SEMANTIC,
        "Unknown expression type in semantic analysis",
        currentLine, 0, "analyzing expression");
    return "error";
}

void SemanticAnalyzer::checkForUnusedVariables() {
    auto unusedVars = symbols.getUnusedVariables();
    for (const std::string& varName : unusedVars) {
        errorHandler.reportWarning(ErrorCategory::SEMANTIC,
            "Variable '" + varName + "' declared but never used",
            0, 0, "checking variable usage");
        errorHandler.addSuggestion("Remove the unused variable or use it in your code");
    }
}


// NEW: Check for unused functions
void SemanticAnalyzer::checkForUnusedFunctions() {
    auto unusedVars = symbols.getUnusedVariables();  // This also includes functions
    for (const std::string& name : unusedVars) {
        const Symbol* symbol = symbols.getSymbol(name);
        if (symbol && symbol->kind == SymbolKind::FUNCTION && name != "print") {
            errorHandler.reportWarning(ErrorCategory::SEMANTIC,
                "Function '" + name + "' declared but never used",
                0, 0, "checking function usage");
            errorHandler.addSuggestion("Remove the unused function or call it in your code");
        }
    }
}



void SemanticAnalyzer::handleFunctionDecl(const FunctionDeclStmt* funcDecl) {
    // Check if function already declared
    if (symbols.isDeclaredInCurrentScope(funcDecl->name)) {
        errorHandler.reportError(ErrorCategory::SEMANTIC,
            "Function '" + funcDecl->name + "' is already declared",
            currentLine, 0, "declaring function");
        return;
    }
    
    // Validate function signature
    validateFunctionSignature(funcDecl->name, funcDecl->parameters, funcDecl->returnType);
    
    // Declare the function
    symbols.declare(funcDecl->name, SymbolKind::FUNCTION);
    std::string signature = createFunctionSignature(funcDecl->parameters, funcDecl->returnType);
    symbols.setType(funcDecl->name, signature);
    
    // Enter new scope for function body
    symbols.enterScope();
    
    // Declare parameters in function scope
    for (const auto& param : funcDecl->parameters) {
        symbols.declare(param.name, SymbolKind::PARAMETER);
        symbols.setType(param.name, param.type);
        symbols.markInitialized(param.name);
    }
    
    // Set function context
    std::string previousFunction = currentFunctionName;
    std::string previousReturnType = currentFunctionReturnType;
    bool previousInFunction = inFunctionBody;
    
    currentFunctionName = funcDecl->name;
    currentFunctionReturnType = funcDecl->returnType;
    inFunctionBody = true;
    
    // Analyze function body
    if (funcDecl->body) {
        for (const auto& stmt : funcDecl->body->statements) {
            if (stmt) {
                analyzeStmt(stmt.get());
            }
        }
    }
    
    // Restore previous context
    currentFunctionName = previousFunction;
    currentFunctionReturnType = previousReturnType;
    inFunctionBody = previousInFunction;
    
    // Exit function scope
    symbols.exitScope();
}

void SemanticAnalyzer::handleReturnStmt(const ReturnStmt* retStmt) {
    if (!inFunctionBody) {
        errorHandler.reportError(ErrorCategory::SEMANTIC,
            "Return statement outside of function",
            currentLine, 0, "analyzing return statement");
        return;
    }
    
    if (retStmt->value) {
        // Return with value
        std::string returnValueType = analyzeExpr(retStmt->value.get());
        if (returnValueType == "error") {
            return;
        }
        
        if (!areTypesCompatible(currentFunctionReturnType, returnValueType)) {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "Return type mismatch: function '" + currentFunctionName + 
                "' expects '" + currentFunctionReturnType + "' but got '" + returnValueType + "'",
                currentLine, 0, "analyzing return statement");
        }
    } else {
        // Return without value (void return)
        if (currentFunctionReturnType != "void") {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "Function '" + currentFunctionName + "' expects return value of type '" + 
                currentFunctionReturnType + "'",
                currentLine, 0, "analyzing return statement");
        }
    }
}


// Add these method implementations to semantic_analyzer.cpp:

bool SemanticAnalyzer::areTypesCompatible(const std::string& expected, const std::string& actual) {
    if (expected == actual) {
        return true;
    }
    
    // Allow implicit conversion from int to double
    if (expected == "double" && actual == "int") {
        return true;
    }
    
    // Array type compatibility - exact match required
    if (isArrayType(expected) && isArrayType(actual)) {
        return expected == actual;  // Arrays must have exact same element type
    }
    
    // Special handling for empty arrays - can be assigned to any array type
    if (expected.find("[") == 0 && actual == "[int]" && expected != "[int]") {
        // This would need more sophisticated handling in a real compiler
        // For now, require exact type matches for arrays
        return false;
    }
    
    return false;
}

std::string SemanticAnalyzer::createFunctionSignature(const std::vector<Parameter>& params, 
                                                       const std::string& returnType) {
    std::string signature = returnType + "(";
    for (size_t i = 0; i < params.size(); ++i) {
        if (i > 0) signature += ",";
        signature += params[i].type;
    }
    signature += ")";
    return signature;
}

void SemanticAnalyzer::validateFunctionSignature(const std::string& name, 
                                                  const std::vector<Parameter>& params,
                                                  const std::string& returnType) {
    // Validate return type (including arrays)
    validateTypeDeclaration(returnType);
    
    // Validate parameter types and names
    for (const auto& param : params) {
        validateTypeDeclaration(param.type);
        
        // Check for duplicate parameter names
        for (const auto& other : params) {
            if (&param != &other && param.name == other.name) {
                errorHandler.reportError(ErrorCategory::SEMANTIC,
                    "Duplicate parameter name '" + param.name + "' in function '" + name + "'",
                    currentLine, 0, "validating function signature");
                break;
            }
        }
    }
}

void SemanticAnalyzer::validateReturnType(const std::string& returnType) {
    if (returnType != "void" && returnType != "int" && returnType != "double" && 
        returnType != "string" && returnType != "bool") {
        errorHandler.reportError(ErrorCategory::SEMANTIC,
            "Invalid type '" + returnType + "'",
            currentLine, 0, "validating type");
        errorHandler.addSuggestion("Use a valid type: int, double, string, bool, or void");
    }
}



void SemanticAnalyzer::handleIndexAssignmentStmt(const IndexAssignmentStmt* indexAssign) {
    // Analyze the object being indexed
    std::string objectType = analyzeExpr(indexAssign->object.get());
    if (objectType == "error") return;
    
    // Check if object is indexable
    if (!isArrayType(objectType) && objectType != "string") {
        errorHandler.reportError(ErrorCategory::SEMANTIC,
            "Cannot index into non-array, non-string type '" + objectType + "'",
            currentLine, 0, "analyzing index assignment");
        errorHandler.addSuggestion("Only arrays and strings can be indexed");
        return;
    }
    
    // Analyze the index
    std::string indexType = analyzeExpr(indexAssign->index.get());
    if (indexType == "error") return;
    
    if (!isValidArrayIndex(indexType)) {
        errorHandler.reportError(ErrorCategory::SEMANTIC,
            "Array index must be an integer, got '" + indexType + "'",
            currentLine, 0, "analyzing index assignment");
        return;
    }
    
    // Analyze the value being assigned
    std::string valueType = analyzeExpr(indexAssign->value.get());
    if (valueType == "error") return;
    
    // Check type compatibility
    std::string expectedType;
    if (isArrayType(objectType)) {
        expectedType = getArrayElementType(objectType);
    } else {
        // String indexing - strings are mutable in our language
        expectedType = "string";  // Individual characters are strings
    }
    
    if (!areTypesCompatible(expectedType, valueType)) {
        errorHandler.reportError(ErrorCategory::SEMANTIC,
            "Cannot assign '" + valueType + "' to " + objectType + " element of type '" + expectedType + "'",
            currentLine, 0, "analyzing index assignment");
        return;
    }
}

std::string SemanticAnalyzer::analyzeArrayLiteral(const ArrayLiteralExpr* arrayLit) {
    if (arrayLit->elements.empty()) {
        // Empty array - type will be inferred from context or default to [int]
        return "[int]";  // Default empty array type
    }
    
    // Analyze first element to determine array type
    std::string elementType = analyzeExpr(arrayLit->elements[0].get());
    if (elementType == "error") return "error";
    
    // Check that all elements have the same type
    for (size_t i = 1; i < arrayLit->elements.size(); ++i) {
        std::string currentType = analyzeExpr(arrayLit->elements[i].get());
        if (currentType == "error") return "error";
        
        if (!areTypesCompatible(elementType, currentType)) {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "Array literal has mixed element types: '" + elementType + 
                "' and '" + currentType + "' at index " + std::to_string(i),
                currentLine, 0, "analyzing array literal");
            errorHandler.addSuggestion("Ensure all array elements have the same type");
            return "error";
        }
    }
    
    return makeArrayType(elementType);  // Returns "[int]", "[string]", etc.
}


std::string SemanticAnalyzer::analyzeIndexExpr(const IndexExpr* indexExpr) {
    // Analyze the object being indexed
    std::string objectType = analyzeExpr(indexExpr->object.get());
    if (objectType == "error") return "error";
    
    // Check if object is indexable
    if (!isArrayType(objectType) && objectType != "string") {
        errorHandler.reportError(ErrorCategory::SEMANTIC,
            "Cannot index into non-array, non-string type '" + objectType + "'",
            currentLine, 0, "analyzing index expression");
        errorHandler.addSuggestion("Only arrays and strings can be indexed");
        return "error";
    }
    
    // Analyze the index
    std::string indexType = analyzeExpr(indexExpr->index.get());
    if (indexType == "error") return "error";
    
    if (!isValidArrayIndex(indexType)) {
        errorHandler.reportError(ErrorCategory::SEMANTIC,
            "Array index must be an integer, got '" + indexType + "'",
            currentLine, 0, "analyzing index expression");
        errorHandler.addSuggestion("Use an integer expression for array indexing");
        return "error";
    }
    
    // Return the element type
    if (isArrayType(objectType)) {
        return getArrayElementType(objectType);  // "[int]" -> "int"
    } else {
        return "string";  // String indexing returns a single character (string)
    }
}



std::string SemanticAnalyzer::analyzeBuiltinCall(const CallExpr* call) {
    const std::string& funcName = call->callee;
    
    if (funcName == "len") {
        if (call->arguments.size() != 1) {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "len() expects exactly one argument, got " + std::to_string(call->arguments.size()),
                currentLine, 0, "analyzing len() call");
            return "error";
        }
        
        std::string argType = analyzeExpr(call->arguments[0].get());
        if (argType == "error") return "error";
        
        if (!isArrayType(argType) && argType != "string") {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "len() requires array or string, got '" + argType + "'",
                currentLine, 0, "analyzing len() call");
            errorHandler.addSuggestion("Use len() with arrays or strings only");
            return "error";
        }
        
        return "int";
    }
    else if (funcName == "push") {
        if (call->arguments.size() != 2) {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "push() expects exactly two arguments, got " + std::to_string(call->arguments.size()),
                currentLine, 0, "analyzing push() call");
            return "error";
        }
        
        std::string arrayType = analyzeExpr(call->arguments[0].get());
        std::string elementType = analyzeExpr(call->arguments[1].get());
        
        if (arrayType == "error" || elementType == "error") return "error";
        
        if (!isArrayType(arrayType)) {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "push() first argument must be an array, got '" + arrayType + "'",
                currentLine, 0, "analyzing push() call");
            return "error";
        }
        
        std::string expectedElementType = getArrayElementType(arrayType);
        if (!areTypesCompatible(expectedElementType, elementType)) {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "push() element type mismatch: array has '" + expectedElementType + 
                "' elements, trying to push '" + elementType + "'",
                currentLine, 0, "analyzing push() call");
            return "error";
        }
        
        return "void";
    }
    else if (funcName == "pop") {
        if (call->arguments.size() != 1) {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "pop() expects exactly one argument, got " + std::to_string(call->arguments.size()),
                currentLine, 0, "analyzing pop() call");
            return "error";
        }
        
        std::string arrayType = analyzeExpr(call->arguments[0].get());
        if (arrayType == "error") return "error";
        
        if (!isArrayType(arrayType)) {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "pop() requires an array, got '" + arrayType + "'",
                currentLine, 0, "analyzing pop() call");
            return "error";
        }
        
        return getArrayElementType(arrayType);  // Returns element type
    }
    else if (funcName == "concat") {
        if (call->arguments.size() != 2) {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "concat() expects exactly two arguments, got " + std::to_string(call->arguments.size()),
                currentLine, 0, "analyzing concat() call");
            return "error";
        }
        
        std::string leftType = analyzeExpr(call->arguments[0].get());
        std::string rightType = analyzeExpr(call->arguments[1].get());
        
        if (leftType == "error" || rightType == "error") return "error";
        
        if (leftType != "string" || rightType != "string") {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "concat() requires two strings, got '" + leftType + "' and '" + rightType + "'",
                currentLine, 0, "analyzing concat() call");
            return "error";
        }
        
        return "string";
    }
    else if (funcName == "substr") {
        if (call->arguments.size() != 3) {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "substr() expects exactly three arguments, got " + std::to_string(call->arguments.size()),
                currentLine, 0, "analyzing substr() call");
            return "error";
        }
        
        std::string strType = analyzeExpr(call->arguments[0].get());
        std::string startType = analyzeExpr(call->arguments[1].get());
        std::string lenType = analyzeExpr(call->arguments[2].get());
        
        if (strType == "error" || startType == "error" || lenType == "error") return "error";
        
        if (strType != "string") {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "substr() first argument must be string, got '" + strType + "'",
                currentLine, 0, "analyzing substr() call");
            return "error";
        }
        
        if (startType != "int" || lenType != "int") {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "substr() start and length must be integers, got '" + startType + "' and '" + lenType + "'",
                currentLine, 0, "analyzing substr() call");
            return "error";
        }
        
        return "string";
    }
    else if (funcName == "input") {
        if (call->arguments.size() != 0) {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "input() expects no arguments, got " + std::to_string(call->arguments.size()),
                currentLine, 0, "analyzing input() call");
            return "error";
        }
        return "string";
    }
    else if (funcName == "println") {
        if (call->arguments.size() != 1) {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "println() expects exactly one argument, got " + std::to_string(call->arguments.size()),
                currentLine, 0, "analyzing println() call");
            return "error";
        }
        
        std::string argType = analyzeExpr(call->arguments[0].get());
        if (argType == "error") return "error";
        
        return "void";
    }
    else if (funcName == "to_string") {
        if (call->arguments.size() != 1) {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "to_string() expects exactly one argument, got " + std::to_string(call->arguments.size()),
                currentLine, 0, "analyzing to_string() call");
            return "error";
        }
        
        std::string argType = analyzeExpr(call->arguments[0].get());
        if (argType == "error") return "error";
        
        return "string";
    }
    else if (funcName == "to_int") {
        if (call->arguments.size() != 1) {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "to_int() expects exactly one argument, got " + std::to_string(call->arguments.size()),
                currentLine, 0, "analyzing to_int() call");
            return "error";
        }
        
        std::string argType = analyzeExpr(call->arguments[0].get());
        if (argType == "error") return "error";
        
        if (argType != "string") {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "to_int() requires string argument, got '" + argType + "'",
                currentLine, 0, "analyzing to_int() call");
            return "error";
        }
        
        return "int";
    }
    else if (funcName == "to_double") {
        if (call->arguments.size() != 1) {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "to_double() expects exactly one argument, got " + std::to_string(call->arguments.size()),
                currentLine, 0, "analyzing to_double() call");
            return "error";
        }
        
        std::string argType = analyzeExpr(call->arguments[0].get());
        if (argType == "error") return "error";
        
        if (argType != "string") {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "to_double() requires string argument, got '" + argType + "'",
                currentLine, 0, "analyzing to_double() call");
            return "error";
        }
        
        return "double";
    }
    else if (funcName == "print") {
        // Existing print analysis
        if (call->arguments.size() != 1) {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "print() expects exactly one argument, got " + std::to_string(call->arguments.size()),
                currentLine, 0, "analyzing print() call");
            return "error";
        }
        
        std::string argType = analyzeExpr(call->arguments[0].get());
        if (argType == "error") return "error";
        
        return "void";
    }
    
    errorHandler.reportError(ErrorCategory::SEMANTIC,
        "Unknown built-in function: " + funcName,
        currentLine, 0, "analyzing built-in call");
    return "error";
}


std::string SemanticAnalyzer::getArrayElementType(const std::string& arrayType) {
    if (!isArrayType(arrayType)) return arrayType;
    
    // "[int]" -> "int", "[[string]]" -> "[string]"
    if (arrayType.size() >= 3 && arrayType[0] == '[' && arrayType.back() == ']') {
        return arrayType.substr(1, arrayType.size() - 2);
    }
    
    return "unknown";
}

std::string SemanticAnalyzer::makeArrayType(const std::string& elementType) {
    return "[" + elementType + "]";
}

bool SemanticAnalyzer::isArrayType(const std::string& type) {
    return type.size() >= 3 && type[0] == '[' && type.back() == ']';
}

int SemanticAnalyzer::getArrayDimensions(const std::string& arrayType) {
    int dimensions = 0;
    for (char c : arrayType) {
        if (c == '[') dimensions++;
        else break;
    }
    return dimensions;
}

bool SemanticAnalyzer::isValidArrayIndex(const std::string& indexType) {
    return indexType == "int";
}

bool SemanticAnalyzer::canConcatenate(const std::string& leftType, const std::string& rightType) {
    return leftType == "string" && rightType == "string";
}

std::string SemanticAnalyzer::inferBinaryResultType(const std::string& leftType, 
                                                    const std::string& rightType, 
                                                    const std::string& op) {
    // String concatenation with +
    if (op == "+" && canConcatenate(leftType, rightType)) {
        return "string";
    }
    
    // Arithmetic operations
    if (op == "+" || op == "-" || op == "*" || op == "/") {
        if (leftType != rightType) {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "Type mismatch in binary expression: '" + leftType + "' " + op + " '" + rightType + "'",
                currentLine, 0, "analyzing binary expression");
            return "error";
        }
        
        if (leftType == "int" || leftType == "double") {
            return leftType;
        } else {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "Arithmetic operator '" + op + "' requires numeric types, got '" + leftType + "'",
                currentLine, 0, "analyzing binary expression");
            return "error";
        }
    }
    
    // Modulo operator
    if (op == "%") {
        if (leftType != rightType || leftType != "int") {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "Modulo operator requires integer operands, got '" + leftType + "' and '" + rightType + "'",
                currentLine, 0, "analyzing binary expression");
            return "error";
        }
        return "int";
    }
    
    // Comparison operators
    if (op == "==" || op == "!=" || op == "<" || op == "<=" || op == ">" || op == ">=") {
        if (!areTypesCompatible(leftType, rightType)) {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "Type mismatch in comparison: '" + leftType + "' " + op + " '" + rightType + "'",
                currentLine, 0, "analyzing binary expression");
            return "error";
        }
        
        if (leftType == "int" || leftType == "double" || leftType == "string") {
            return "bool";
        } else {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "Comparison operator '" + op + "' requires numeric or string types, got '" + leftType + "'",
                currentLine, 0, "analyzing binary expression");
            return "error";
        }
    }
    
    // Logical operators
    if (op == "&&" || op == "||") {
        if ((leftType == "int" || leftType == "double" || leftType == "bool") && 
            (rightType == "int" || rightType == "double" || rightType == "bool")) {
            return "bool";
        } else {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "Logical operator '" + op + "' requires boolean-convertible types",
                currentLine, 0, "analyzing binary expression");
            return "error";
        }
    }
    
    errorHandler.reportError(ErrorCategory::SEMANTIC,
        "Unknown binary operator: " + op,
        currentLine, 0, "analyzing binary expression");
    return "error";
}



void SemanticAnalyzer::validateTypeDeclaration(const std::string& typeDecl) {
    if (typeDecl.empty()) return;
    
    // Validate basic types
    if (typeDecl == "int" || typeDecl == "double" || typeDecl == "string" || 
        typeDecl == "bool" || typeDecl == "void") {
        return;
    }
    
    // Validate array types
    if (isArrayType(typeDecl)) {
        std::string elementType = getArrayElementType(typeDecl);
        validateTypeDeclaration(elementType);  // Recursive validation
        return;
    }
    
    // Unknown type
    errorHandler.reportError(ErrorCategory::SEMANTIC,
        "Unknown type '" + typeDecl + "' in declaration",
        currentLine, 0, "validating type declaration");
    errorHandler.addSuggestion("Use valid types like 'int', 'string', '[int]', or nested arrays like '[[string]]'");
}

void SemanticAnalyzer::debugPrintType(const std::string& type, const std::string& context) {
    std::cout << "DEBUG [" << context << "]: Type = '" << type << "'";
    if (isArrayType(type)) {
        std::cout << " (Array, element type: '" << getArrayElementType(type) 
                  << "', dimensions: " << getArrayDimensions(type) << ")";
    }
    std::cout << std::endl;
}