#include "semantic_analyzer.h"

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
    symbols.declare("print", SymbolKind::FUNCTION);
    symbols.setType("print", "void(any)");
    symbols.markUsed("print");
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
    // ADD THESE TWO CASES:
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
    
    // Then analyze the initializer and set the type
    if (varDecl->initializer) {
        std::string initializerType = analyzeExpr(varDecl->initializer.get());
        if (initializerType != "error") {  // Only set type if analysis succeeded
            symbols.setType(varDecl->name, initializerType);
            symbols.markInitialized(varDecl->name);
        }
    } else {
        // Default initialization
        symbols.setType(varDecl->name, "int");
        symbols.markInitialized(varDecl->name);
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
        return;  // Error already reported in expression analysis
    }
    
    // Check type compatibility
    if (varType != valueType) {
        errorHandler.reportError(ErrorCategory::SEMANTIC,
            "Type mismatch in assignment: cannot assign '" + valueType + "' to variable '" + 
            assignStmt->name + "' of type '" + varType + "'",
            currentLine, 0, "analyzing assignment");
        errorHandler.addSuggestion("Ensure the assigned value matches the variable's type");
        return;
    }
    
    // Mark variable as used (since we're reading its type) and ensure it's initialized
    symbols.markUsed(assignStmt->name);
    
    // The assignment itself acts as initialization
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
    if (!expr) return "error";  // Handle null expressions from parser errors
    
    if (auto varExpr = dynamic_cast<const VariableExpr*>(expr)) {
        if (!symbols.isDeclared(varExpr->name)) {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "Use of undeclared variable '" + varExpr->name + "'",
                currentLine, 0, "analyzing expression");
            
            // Could add variable name suggestions here
            return "error";
        }
        
        // Check if variable is initialized before use
        const Symbol* symbol = symbols.getSymbol(varExpr->name);
        if (symbol && symbol->kind == SymbolKind::VARIABLE && !symbol->isInitialized) {
            errorHandler.reportWarning(ErrorCategory::SEMANTIC,
                "Variable '" + varExpr->name + "' may be used before initialization",
                currentLine, 0, "analyzing variable usage");
            errorHandler.addSuggestion("Initialize the variable before using it");
        }
        
        // Mark variable as used
        symbols.markUsed(varExpr->name);
        
        return symbols.getType(varExpr->name);
    }
    else if (auto literal = dynamic_cast<const LiteralExpr*>(expr)) {
        const std::string& v = literal->value;
        
        // Handle boolean literals
        if (v == "0" || v == "1") {
            // Could be boolean or integer - treat as int for now
            return "int";
        }
        
        bool hasDot = false, allDigitsOrDot = !v.empty();
        for (char c : v) {
            if (c == '.') { hasDot = true; continue; }
            if (c < '0' || c > '9') { allDigitsOrDot = false; break; }
        }
        if (allDigitsOrDot && hasDot) return "double";
        if (allDigitsOrDot && !hasDot) return "int";
        return "string";
    }
    else if (auto unary = dynamic_cast<const UnaryExpr*>(expr)) {
        std::string operandType = analyzeExpr(unary->operand.get());
        
        if (operandType == "error") {
            return "error";  // Propagate errors
        }
        
        if (unary->op == "-") {
            if (operandType == "int" || operandType == "double") {
                return operandType;  // Unary minus preserves numeric type
            } else {
                errorHandler.reportError(ErrorCategory::SEMANTIC,
                    "Unary minus operator '-' requires numeric type, got '" + operandType + "'",
                    currentLine, 0, "analyzing unary expression");
                return "error";
            }
        } else if (unary->op == "!") {
            // Logical NOT always returns boolean (represented as int)
            return "bool";
        } else {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "Unknown unary operator: " + unary->op,
                currentLine, 0, "analyzing unary expression");
            return "error";
        }
    }
    else if (auto binary = dynamic_cast<const BinaryExpr*>(expr)) {
        std::string leftType = analyzeExpr(binary->left.get());
        std::string rightType = analyzeExpr(binary->right.get());
        
        if (leftType == "error" || rightType == "error") {
            return "error";  // Propagate errors
        }
        
        // Arithmetic operators (+, -, *, /, %)
        if (binary->op == "+" || binary->op == "-" || binary->op == "*" || binary->op == "/") {
            if (leftType != rightType) {
                errorHandler.reportError(ErrorCategory::SEMANTIC,
                    "Type mismatch in binary expression: '" + leftType + "' " + binary->op + " '" + rightType + "'",
                    currentLine, 0, "analyzing binary expression");
                errorHandler.addSuggestion("Ensure both operands have the same type");
                return "error";
            }
            
            if (leftType == "int" || leftType == "double") {
                return leftType;  // Result has same type as operands
            } else {
                errorHandler.reportError(ErrorCategory::SEMANTIC,
                    "Arithmetic operator '" + binary->op + "' requires numeric types, got '" + leftType + "'",
                    currentLine, 0, "analyzing binary expression");
                return "error";
            }
        }
        // Modulo operator (%)
        else if (binary->op == "%") {
            if (leftType != rightType) {
                errorHandler.reportError(ErrorCategory::SEMANTIC,
                    "Type mismatch in modulo expression: '" + leftType + "' % '" + rightType + "'",
                    currentLine, 0, "analyzing binary expression");
                return "error";
            }
            
            if (leftType == "int") {
                return "int";
            } else {
                errorHandler.reportError(ErrorCategory::SEMANTIC,
                    "Modulo operator '%' requires integer types, got '" + leftType + "'",
                    currentLine, 0, "analyzing binary expression");
                return "error";
            }
        }
        // Comparison operators (==, !=, <, <=, >, >=)
else if (binary->op == "==" || binary->op == "!=" || 
         binary->op == "<" || binary->op == "<=" || 
         binary->op == ">" || binary->op == ">=") {
    if (!areTypesCompatible(leftType, rightType)) {
        errorHandler.reportError(ErrorCategory::SEMANTIC,
            "Type mismatch in comparison: '" + leftType + "' " + binary->op + " '" + rightType + "'",
            currentLine, 0, "analyzing binary expression");
        return "error";
    }
    
    if (leftType == "int" || leftType == "double") {
        return "bool";  // CHANGED: Return "bool" instead of "int"
    } else {
        errorHandler.reportError(ErrorCategory::SEMANTIC,
            "Comparison operator '" + binary->op + "' requires numeric types, got '" + leftType + "'",
            currentLine, 0, "analyzing binary expression");
        return "error";
    }
}
        // Logical operators (&&, ||)
else if (binary->op == "&&" || binary->op == "||") {
    if ((leftType == "int" || leftType == "double" || leftType == "bool") && 
        (rightType == "int" || rightType == "double" || rightType == "bool")) {
        return "bool";  // CHANGED: Return "bool" instead of "int"
    } else {
        errorHandler.reportError(ErrorCategory::SEMANTIC,
            "Logical operator '" + binary->op + "' requires boolean-convertible types",
            currentLine, 0, "analyzing binary expression");
        return "error";
    }
}
        else {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "Unknown binary operator: " + binary->op,
                currentLine, 0, "analyzing binary expression");
            return "error";
        }
    }
    else if (auto call = dynamic_cast<const CallExpr*>(expr)) {
        if (!symbols.isDeclared(call->callee)) {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "Call to undeclared function '" + call->callee + "'",
                currentLine, 0, "analyzing function call");
            return "error";
        }
        
        symbols.markUsed(call->callee);
        
        if (call->callee == "print") {
            if (call->arguments.size() != 1) {
                errorHandler.reportError(ErrorCategory::SEMANTIC,
                    "print() expects exactly one argument, got " + std::to_string(call->arguments.size()),
                    currentLine, 0, "analyzing function call");
                errorHandler.addSuggestion("Provide exactly one argument to print()");
                return "error";
            }
            
            // Analyze the argument
            std::string argType = analyzeExpr(call->arguments[0].get());
            if (argType == "error") {
                return "error";
            }
            
            return "void";
        }
        if (symbols.isFunction(call->callee)) {
    std::string returnType = symbols.getFunctionReturnType(call->callee);
    
    // Analyze arguments
    for (const auto& arg : call->arguments) {
        std::string argType = analyzeExpr(arg.get());
        if (argType == "error") return "error";
    }
    
    return returnType;  // Returns "int", not "int(int)"
}
        
        return symbols.getType(call->callee);
    }
    // NEW: Handle range expressions
    else if (auto rangeExpr = dynamic_cast<const RangeExpr*>(expr)) {
    std::string startType = analyzeExpr(rangeExpr->start.get());
    std::string endType = analyzeExpr(rangeExpr->end.get());
    
    if (startType == "error" || endType == "error") {
        return "error";  // Propagate errors
    }
    
    // Both start and end should be integers
    if (startType != "int" || endType != "int") {
        errorHandler.reportError(ErrorCategory::SEMANTIC,
            "Range expressions require integer start and end values, got '" + 
            startType + "' and '" + endType + "'",
            currentLine, 0, "analyzing range expression");
        errorHandler.addSuggestion("Use integer values for range bounds like '0:10'");
        return "error";
    }
    
    // NEW: Phase 2 - Validate step if provided
    if (rangeExpr->step) {
        std::string stepType = analyzeExpr(rangeExpr->step.get());
        if (stepType == "error") {
            return "error";
        }
        if (stepType != "int") {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "Range step must be an integer, got '" + stepType + "'",
                currentLine, 0, "analyzing range expression");
            errorHandler.addSuggestion("Use integer step values like '0:10:2'");
            return "error";
        }
        
        // Additional validation: step cannot be zero
        // Note: We could add compile-time validation for literal steps
        if (auto stepLiteral = dynamic_cast<const LiteralExpr*>(rangeExpr->step.get())) {
            if (stepLiteral->value == "0") {
                errorHandler.reportError(ErrorCategory::SEMANTIC,
                    "Range step cannot be zero",
                    currentLine, 0, "analyzing range expression");
                errorHandler.addSuggestion("Use a non-zero step value like '1' or '-1'");
                return "error";
            }
        }
    }
    
    return "range";  // Return range type
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
    
    // Allow bool to be treated as int in certain contexts (like conditions)
    // But keep them distinct for function return types
    
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
    // Validate return type
    validateReturnType(returnType);
    
    // Validate parameter types and names
    for (const auto& param : params) {
        validateReturnType(param.type);  // Same validation for parameter types
        
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