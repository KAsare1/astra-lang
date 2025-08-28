// === main.cpp - Updated to use shared symbol table ===
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

#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: astra <source_file>\n";
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "Error: Cannot open file " << argv[1] << "\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    // ========== SINGLE SHARED SYMBOL TABLE ==========
    SymbolTable globalSymbolTable;
    
    std::cout << "=== PHASE 1: LEXICAL ANALYSIS ===\n";
    // Phase 1: Lexer creates entries for identifiers
    Lexer lexer(source, globalSymbolTable);
    std::vector<Token> tokens;
    try {
        tokens = lexer.tokenize();
        std::cout << "Lexical analysis completed. Tokens generated: " << tokens.size() << "\n";
    } catch (const std::runtime_error& e) {
        std::cerr << "Lexer error: " << e.what() << "\n";
        return 1;
    }

    std::cout << "\n=== PHASE 2: SYNTAX ANALYSIS ===\n";
    // Phase 2: Parser adds attribute information
    Parser parser(tokens, globalSymbolTable);
    auto statements = parser.parse();
    std::cout << "Syntax analysis completed. Parsed " << statements.size() << " statements.\n";

    std::cout << "\n=== PHASE 3: SEMANTIC ANALYSIS ===\n";
    // Phase 3: Semantic analyzer performs type checking
    SemanticAnalyzer analyzer(globalSymbolTable);
    try {
        analyzer.analyze(statements);
        std::cout << "Semantic analysis passed.\n";
    } catch (const std::runtime_error& e) {
        std::cerr << "Semantic error: " << e.what() << "\n";
        return 1;
    }

    // Print symbol table state after semantic analysis
    std::cout << "\n=== SYMBOL TABLE STATE ===\n";
    globalSymbolTable.print();

    std::cout << "\n=== PHASE 4: INTERMEDIATE CODE GENERATION ===\n";
    // Phase 4: IR generator uses symbol table for runtime allocation
    try {
        IRCodegen irgen("AstraModule", globalSymbolTable);  // Pass the same symbol table
        irgen.emit(statements);

        llvm::Module &module = irgen.getModule();

        // Write LLVM IR to file
        std::error_code ec;
        llvm::raw_fd_ostream irOut("build/output.ll", ec, llvm::sys::fs::OF_Text);
        if (ec) {
            std::cerr << "Error opening build/output.ll: " << ec.message() << "\n";
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
        std::cerr << "Codegen error: " << e.what() << "\n";
        return 1;
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