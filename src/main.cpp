#include <iostream>
#include <fstream>
#include <sstream>

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "abstract-syntax-tree/ast_printer.h"
#include "semantic/semantic_analyzer.h"
#include "code-generator/ir_codegen.h"
#include "code-generator/target/target_codegen.h" 
#include "shared/symbol_table.h"
#include "shared/error_handler.h"  // Add error handler

#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"


using namespace std;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: astra <source_file>\n";
        return 1;
    }

    // Initialize error handler firs
    ErrorHandler errorHandler;

    std::ifstream file(argv[1]);
    if (!file) {
        errorHandler.reportFatal(ErrorCategory::SYSTEM, 
            "Cannot open file " + std::string(argv[1]));
        errorHandler.printMessages();
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    // Set up error handler with source file
    errorHandler.setSourceFile(argv[1], source);
    
    // ========== SINGLE SHARED SYMBOL TABLE ==========
    SymbolTable globalSymbolTable;
    
    std::cout << "=== PHASE 1: LEXICAL ANALYSIS ===\n";
    // Phase 1: Lexer creates entries for identifiers
    Lexer lexer(source, globalSymbolTable, errorHandler);
    std::vector<Token> tokens;
    
    try {
        tokens = lexer.tokenize();
        std::cout << "Lexical analysis completed. Tokens generated: " << tokens.size() << "\n";
        
        // DEBUG: Print first few tokens
        std::cout << "First 20 tokens:\n";
        for (size_t i = 0; i < std::min(tokens.size(), size_t(20)); ++i) {
            std::cout << "  " << tokenTypeToString(tokens[i].type) << ": '" << tokens[i].lexeme << "'\n";
        }
        
    } catch (const std::runtime_error& e) {
        // Lexer should have reported errors through errorHandler, but catch any that slip through
        if (!errorHandler.hasErrorsOccurred()) {
            errorHandler.reportError(ErrorCategory::LEXICAL, e.what());
        }
    }

    // Check for lexical errors before continuing
    if (errorHandler.hasErrorsOccurred()) {
        errorHandler.printMessages();
        errorHandler.printSummary();
        return 1;
    }

    std::cout << "\n=== PHASE 2: SYNTAX ANALYSIS ===\n";
    // Phase 2: Parser adds attribute information
    Parser parser(tokens, globalSymbolTable, errorHandler);
    std::vector<std::unique_ptr<Stmt>> statements;
    
    try {
        statements = parser.parse();
        std::cout << "Syntax analysis completed. Parsed " << statements.size() << " statements.\n";
        
        // DEBUG: Print AST
        std::cout << "\n=== AST STRUCTURE ===\n";
        for (size_t i = 0; i < statements.size(); ++i) {
            std::cout << "Statement " << i << ":\n";
            printStmt(statements[i].get(), 1);
        }
        
    } catch (const std::runtime_error& e) {
        // Parser should have reported errors through errorHandler
        if (!errorHandler.hasErrorsOccurred()) {
            errorHandler.reportError(ErrorCategory::SYNTAX, e.what());
        }
    }

    // Check for syntax errors
    if (errorHandler.hasErrorsOccurred()) {
        errorHandler.printMessages();
        errorHandler.printSummary();
        return 1;
    }

    std::cout << "\n=== PHASE 3: SEMANTIC ANALYSIS ===\n";
    // Phase 3: Semantic analyzer performs type checking
    SemanticAnalyzer analyzer(globalSymbolTable, errorHandler);
    
    try {
        analyzer.analyze(statements);
        std::cout << "Semantic analysis completed.\n";
    } catch (const std::runtime_error& e) {
        // Semantic analyzer should have reported errors through errorHandler
        if (!errorHandler.hasErrorsOccurred()) {
            errorHandler.reportError(ErrorCategory::SEMANTIC, e.what());
        }
    }

    // Print any warnings/errors found during semantic analysis
    if (errorHandler.getWarningCount() > 0 || errorHandler.hasErrorsOccurred()) {
        errorHandler.printMessages();
    }

    // Stop if there are errors (but continue if only warnings)
    if (errorHandler.hasErrorsOccurred()) {
        errorHandler.printSummary();
        return 1;
    }

    // Print symbol table state after semantic analysis
    std::cout << "\n=== SYMBOL TABLE STATE ===\n";
    globalSymbolTable.print();

    std::cout << "\n=== PHASE 4: INTERMEDIATE CODE GENERATION ===\n";
    // Phase 4: IR generator uses symbol table for runtime allocation
    try {
        IRCodegen irgen("AstraModule", globalSymbolTable, errorHandler);
        irgen.emit(statements);

        llvm::Module &module = irgen.getModule();

        // Write LLVM IR to file
        std::error_code ec;
        llvm::raw_fd_ostream irOut("build/output.ll", ec, llvm::sys::fs::OF_Text);
        if (ec) {
            errorHandler.reportError(ErrorCategory::SYSTEM, 
                "Error opening build/output.ll: " + ec.message());
            errorHandler.printMessages();
            return 1;
        }
        module.print(irOut, nullptr);
        std::cout << "Wrote LLVM IR to build/output.ll\n";

        std::cout << "\n=== PHASE 5: TARGET CODE GENERATION ===\n";
        // Phase 5: Target code generation
        TargetCodegen targetGen(module);
        targetGen.emitAssembly("build/output.s");   
        targetGen.emitObject("build/output.o");     
        std::cout << "Wrote assembly to build/output.s and object to build/output.o\n";

        std::cout << "\n=== PHASE 6: CODE OPTIMIZATION ===\n";
        // Future: Use symbol table information for optimization passes
        std::cout << "Optimization passes skipped (not implemented yet)\n";

    } catch (const std::exception& e) {
        errorHandler.reportError(ErrorCategory::CODEGEN, e.what());
        errorHandler.printMessages();
        errorHandler.printSummary();
        return 1;
    }

    // Print any final warnings/messages
    if (errorHandler.getWarningCount() > 0) {
        errorHandler.printMessages();
    }

    std::cout << "\n=== COMPILATION SUMMARY ===\n";
    std::cout << "✓ All phases completed successfully\n";
    std::cout << "✓ Symbol table maintained throughout pipeline\n";
    
    // Final symbol table analysis
    auto unusedVars = globalSymbolTable.getUnusedVariables();
    if (!unusedVars.empty()) {
        std::cout << "⚠ Unused variables detected: ";
        for (const auto& var : unusedVars) {
            std::cout << var << " ";
        }
        std::cout << "\n";
    } else {
        std::cout << "✓ No unused variables found\n";
    }
    
    errorHandler.printSummary();
    
    std::cout << "\nGenerated files:\n";
    std::cout << "  - build/output.ll (LLVM IR)\n";
    std::cout << "  - build/output.s  (Assembly)\n"; 
    std::cout << "  - build/output.o  (Object file)\n";
    std::cout << "\nTo create executable:\n";
    std::cout << "  gcc -c src/code-generator/runtime.c -o build/runtime.o\n";
    std::cout << "  gcc build/output.o build/runtime.o -o executable\n";
    std::cout << "  ./executable\n";

    return 0;
}