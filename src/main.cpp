#include <iostream>
#include <fstream>
#include <sstream>

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "abstract-syntax-tree/ast_printer.h"
#include "semantic/semantic_analyzer.h"
#include "code-generator/ir_codegen.h"
// #include "shared/symbol_table.h"

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
    std::cout << "Parsed " << statements.size() << " statements.\n";

    // Semantic
    SemanticAnalyzer analyzer;
    try {
        analyzer.analyze(statements);
        std::cout << "Semantic analysis passed.\n";
    } catch (const std::runtime_error& e) {
        std::cerr << "Semantic error: " << e.what() << "\n";
        return 1;
    }

    // AST (debug)
    std::cout << "\n===== AST =====\n";
    for (const auto& stmt : statements) {
        printStmt(stmt.get());
    }

    // Tokens (debug)
    std::cout << "\n===== TOKENS =====\n";
    for (const auto& token : tokens) {
        std::cout << "("
                  << tokenTypeToString(token.type) << ", "
                  << "\"" << token.lexeme << "\", "
                  << token.line << ", "
                  << token.column
                  << ")\n";
    }

    // Visualize SymbolTable (debug)
    std::cout << "\n===== SYMBOL TABLE =====\n";
    // If SemanticAnalyzer does not expose the symbol table, comment out or implement getSymbolTable()
    analyzer.getSymbolTable().print();

    // ---------- IR Generation ----------
    try {
        SymbolTable syms; // if you maintain a global one from semantic, pass that instead
        IRCodegen codegen("AstraModule", syms);
        codegen.emit(statements);

        // Print to stderr for quick inspection
        codegen.getModule().print(llvm::errs(), nullptr);

        // Write to file
        std::error_code ec;
        llvm::raw_fd_ostream out("build/output.ll", ec, llvm::sys::fs::OF_Text);
        if (ec) {
            std::cerr << "Error opening build/output.ll: " << ec.message() << "\n";
            return 1;
        }
        codegen.getModule().print(out, nullptr);
        std::cout << "\nWrote IR to build/output.ll\n";
    } catch (const std::exception& e) {
        std::cerr << "Codegen error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
