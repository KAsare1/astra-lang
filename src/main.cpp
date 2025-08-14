#include <iostream>
#include <fstream>
#include <sstream>

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "abstract-syntax-tree/ast_printer.h"
#include "semantic/semantic_analyzer.h"


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

    Lexer lexer(source);
    std::vector<Token> tokens;

    try {
        tokens = lexer.tokenize();
    } catch (const std::runtime_error& e) {
        std::cerr << "Lexer error: " << e.what() << "\n";
        return 1;
    }

    Parser parser(tokens);
    auto statements = parser.parse();

    std::cout << "Parsed " << statements.size() << " statements." << std::endl;


        SemanticAnalyzer analyzer;
    try {
        analyzer.analyze(statements);
        std::cout << "Semantic analysis passed.\n";
    } catch (const std::runtime_error& e) {
        std::cerr << "Semantic error: " << e.what() << "\n";
        return 1;
    }     

    
    std::cout << "\n===== AST =====\n";
    for (const auto& stmt : statements) {
        printStmt(stmt.get());
    }

    std::cout << "\n===== TOKENS =====\n";
    for (const auto& token : tokens) {
        std::cout << "(" 
                << tokenTypeToString(token.type) << ", "
                << "\"" << token.lexeme << "\", "
                << token.line << ", "
                << token.column
                << ")\n";
    }

    return 0;
}
