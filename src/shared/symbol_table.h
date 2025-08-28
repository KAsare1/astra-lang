#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>

enum class SymbolKind {
    VARIABLE,
    FUNCTION,
    PARAMETER,
    TEMPORARY
};

struct Symbol {
    std::string name;
    std::string type;           // e.g., "int", "double", "string", "void(int)"
    SymbolKind kind;            // What kind of symbol this is
    int scopeLevel;             // Which scope level it was declared in
    int lineNumber;             // Line where it was declared
    bool isInitialized;         // Whether it has been given a value
    bool isUsed;                // Whether it has been referenced
    size_t memorySize;          // Size in bytes (for code generation)
    std::string llvmType;       // LLVM type string (set during IR generation)
    
    // Constructor
    Symbol(const std::string& n = "", SymbolKind k = SymbolKind::VARIABLE) 
        : name(n), kind(k), scopeLevel(0), lineNumber(0), 
          isInitialized(false), isUsed(false), memorySize(0) {}
};

class SymbolTable {
private:
    std::vector<std::unordered_map<std::string, Symbol>> scopes;
    int currentScopeLevel;

public:
    SymbolTable() : currentScopeLevel(0) {
        enterScope(); // Start with global scope
    }

    // === Phase 1: Lexical Analysis ===
    // Creates new table entries for identifiers
    void declareIdentifier(const std::string& name, int lineNumber = 0) {
        if (scopes.empty()) {
            throw std::runtime_error("No scope available to declare identifier: " + name);
        }
        
        // Only add if not already in current scope
        if (scopes.back().find(name) == scopes.back().end()) {
            Symbol symbol(name, SymbolKind::VARIABLE);
            symbol.scopeLevel = currentScopeLevel;
            symbol.lineNumber = lineNumber;
            scopes.back()[name] = symbol;
        }
    }

    // === Phase 2: Syntax Analysis ===
    // Adds attribute information (type, scope, etc.)
    void declare(const std::string& name, SymbolKind kind = SymbolKind::VARIABLE) {
        if (scopes.empty()) {
            throw std::runtime_error("No scope available to declare symbol: " + name);
        }
        
        // Check if already declared in current scope
        if (scopes.back().find(name) != scopes.back().end()) {
            // Already exists, just update the kind if needed
            scopes.back()[name].kind = kind;
        } else {
            // Create new symbol
            Symbol symbol(name, kind);
            symbol.scopeLevel = currentScopeLevel;
            scopes.back()[name] = symbol;
        }
    }

    // === Phase 3: Semantic Analysis ===
    // Type checking and semantic verification
    void setType(const std::string& name, const std::string& type) {
        Symbol* symbol = findSymbol(name);
        if (!symbol) {
            throw std::runtime_error("Cannot set type for undeclared symbol: " + name);
        }
        symbol->type = type;
        
        // Set memory size based on type
        if (type == "int" || type == "i64") {
            symbol->memorySize = 8;
        } else if (type == "double") {
            symbol->memorySize = 8;
        } else if (type == "string" || type.find("ptr") != std::string::npos) {
            symbol->memorySize = 8; // pointer size
        }
    }

    void markInitialized(const std::string& name) {
        Symbol* symbol = findSymbol(name);
        if (symbol) {
            symbol->isInitialized = true;
        }
    }

    void markUsed(const std::string& name) {
        Symbol* symbol = findSymbol(name);
        if (symbol) {
            symbol->isUsed = true;
        }
    }

    // === Phase 4: Intermediate Code Generation ===
    // Runtime allocation information
    void setLLVMType(const std::string& name, const std::string& llvmType) {
        Symbol* symbol = findSymbol(name);
        if (symbol) {
            symbol->llvmType = llvmType;
        }
    }

    // === Scope Management ===
    void enterScope() {
        scopes.emplace_back();
        currentScopeLevel++;
    }

    void exitScope() {
        if (scopes.empty()) {
            throw std::runtime_error("No scope to exit.");
        }
        scopes.pop_back();
        currentScopeLevel--;
    }

    // === Query Methods ===
    bool isDeclared(const std::string& name) const {
        return findSymbol(name) != nullptr;
    }

    bool isDeclaredInCurrentScope(const std::string& name) const {
        if (scopes.empty()) return false;
        return scopes.back().find(name) != scopes.back().end();
    }

    std::string getType(const std::string& name) const {
        const Symbol* symbol = findSymbol(name);
        if (!symbol) {
            throw std::runtime_error("Symbol not found: " + name);
        }
        return symbol->type;
    }

    Symbol* getSymbol(const std::string& name) {
        return findSymbol(name);
    }

    const Symbol* getSymbol(const std::string& name) const {
        return findSymbol(name);
    }

    // Get all symbols in current scope (useful for code generation)
    std::vector<Symbol*> getCurrentScopeSymbols() {
        std::vector<Symbol*> symbols;
        if (!scopes.empty()) {
            for (auto& [name, symbol] : scopes.back()) {
                symbols.push_back(&symbol);
            }
        }
        return symbols;
    }

    // === Debug and Analysis ===
    void print() const {
        std::cout << "Symbol Table (Current Scope Level: " << currentScopeLevel << "):" << std::endl;
        for (size_t i = 0; i < scopes.size(); ++i) {
            std::cout << "Scope " << i << ":" << std::endl;
            for (const auto& [name, symbol] : scopes[i]) {
                std::cout << "  " << name 
                         << " -> Type: " << symbol.type
                         << ", Kind: " << static_cast<int>(symbol.kind)
                         << ", Line: " << symbol.lineNumber
                         << ", Initialized: " << (symbol.isInitialized ? "Yes" : "No")
                         << ", Used: " << (symbol.isUsed ? "Yes" : "No")
                         << std::endl;
            }
        }
    }

    // Analyze unused variables
    std::vector<std::string> getUnusedVariables() const {
        std::vector<std::string> unused;
        for (const auto& scope : scopes) {
            for (const auto& [name, symbol] : scope) {
                if (symbol.kind == SymbolKind::VARIABLE && !symbol.isUsed) {
                    unused.push_back(name);
                }
            }
        }
        return unused;
    }

private:
    Symbol* findSymbol(const std::string& name) {
        // Search from innermost to outermost scope
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) {
                return &found->second;
            }
        }
        return nullptr;
    }

    const Symbol* findSymbol(const std::string& name) const {
        // Search from innermost to outermost scope
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) {
                return &found->second;
            }
        }
        return nullptr;
    }
};