#include <iostream>
#include <fstream>
#include <sstream>

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "abstract-syntax-tree/ast_printer.h"
#include "semantic/semantic_analyzer.h"
#include "code-generator/ir_codegen.h"
#include "code-generator/target/target_codegen.h" 

#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: astra <source_file>\n";
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "Error: Cannot open file xoxox " << argv[1] << "\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    // Lex
    Lexer lexer(source);
    std::vector<Token> tokens;
    try {
        tokens = lexer.tokenize();
    } catch (const std::runtime_error& e) {
        std::cerr << "Lexer error: " << e.what() << "\n";
        return 1;
    }

    // Parse
    Parser parser(tokens);
    auto statements = parser.parse();
    std::cout << "Parsed" << statements.size() << " statements.\n";

    // Semantic
    SemanticAnalyzer analyzer;
    try {
        analyzer.analyze(statements);
        std::cout << "Semantic analysis passed.\n";
    } catch (const std::runtime_error& e) {
        std::cerr << "Semantic error: " << e.what() << "\n";
        return 1;
    }

    // ---------- IR Generation ----------
    try {
        SymbolTable syms; 
        IRCodegen irgen("AstraModule", syms);
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

        // ---------- Target Code Generation ----------
        TargetCodegen targetGen(module);
        targetGen.emitAssembly("build/output.s");   // 🔹 emits assembly
        targetGen.emitObject("build/output.o");     // 🔹 emits object code
        std::cout << "Wrote assembly to build/output.s and object to build/output.o\n";

    } catch (const std::exception& e) {
        std::cerr << "Codegen error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
