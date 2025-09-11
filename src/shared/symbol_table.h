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
    std::string type;           // e.g., "int", "void(int,double)", "string"
    SymbolKind kind;            
    int scopeLevel;             
    int lineNumber;             
    bool isInitialized;         
    bool isUsed;                
    size_t memorySize;          
    std::string llvmType;       
    
    // NEW: Function-specific information
    std::vector<std::string> parameterTypes;  // For functions
    std::string returnType;                   // For functions
    bool isBuiltin;                          // True for built-in functions
    
    Symbol(const std::string& n = "", SymbolKind k = SymbolKind::VARIABLE) 
        : name(n), kind(k), scopeLevel(0), lineNumber(0), 
          isInitialized(false), isUsed(false), memorySize(0), isBuiltin(false) {}
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
    void declareIdentifier(const std::string& name, int lineNumber = 0) {
        if (scopes.empty()) {
            throw std::runtime_error("No scope available to declare identifier: " + name);
        }
        
        if (scopes.back().find(name) == scopes.back().end()) {
            Symbol symbol(name, SymbolKind::VARIABLE);
            symbol.scopeLevel = currentScopeLevel;
            symbol.lineNumber = lineNumber;
            scopes.back()[name] = symbol;
        }
    }

    // === Phase 2: Syntax Analysis ===
    void declare(const std::string& name, SymbolKind kind = SymbolKind::VARIABLE) {
        if (scopes.empty()) {
            throw std::runtime_error("No scope available to declare symbol: " + name);
        }
        
        if (scopes.back().find(name) != scopes.back().end()) {
            scopes.back()[name].kind = kind;
        } else {
            Symbol symbol(name, kind);
            symbol.scopeLevel = currentScopeLevel;
            scopes.back()[name] = symbol;
        }
    }

    // === Phase 3: Semantic Analysis ===
    void setType(const std::string& name, const std::string& type) {
        Symbol* symbol = findSymbol(name);
        if (!symbol) {
            throw std::runtime_error("Cannot set type for undeclared symbol: " + name);
        }
        symbol->type = type;
        
        // NEW: Parse function signatures
        if (symbol->kind == SymbolKind::FUNCTION) {
            parseFunctionSignature(type, symbol->returnType, symbol->parameterTypes);
        }
        
        // Set memory size based on type
        if (type == "int" || type == "i64") {
            symbol->memorySize = 8;
        } else if (type == "double") {
            symbol->memorySize = 8;
        } else if (type == "string" || type.find("ptr") != std::string::npos) {
            symbol->memorySize = 8; // pointer size
        } else if (type == "bool") {
            symbol->memorySize = 1;
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

    // NEW: Mark as builtin function
    void markBuiltin(const std::string& name) {
        Symbol* symbol = findSymbol(name);
        if (symbol) {
            symbol->isBuiltin = true;
        }
    }

    // === Phase 4: Intermediate Code Generation ===
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

    // NEW: Function-specific queries
    bool isFunction(const std::string& name) const {
        const Symbol* symbol = findSymbol(name);
        return symbol && symbol->kind == SymbolKind::FUNCTION;
    }

    std::vector<std::string> getFunctionParameterTypes(const std::string& name) const {
        const Symbol* symbol = findSymbol(name);
        if (symbol && symbol->kind == SymbolKind::FUNCTION) {
            return symbol->parameterTypes;
        }
        return {};
    }

    std::string getFunctionReturnType(const std::string& name) const {
        const Symbol* symbol = findSymbol(name);
        if (symbol && symbol->kind == SymbolKind::FUNCTION) {
            return symbol->returnType;
        }
        return "void";
    }

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
                         << ", Kind: " << kindToString(symbol.kind)
                         << ", Line: " << symbol.lineNumber
                         << ", Initialized: " << (symbol.isInitialized ? "Yes" : "No")
                         << ", Used: " << (symbol.isUsed ? "Yes" : "No");
                
                if (symbol.kind == SymbolKind::FUNCTION) {
                    std::cout << ", ReturnType: " << symbol.returnType;
                    std::cout << ", Params: [";
                    for (size_t j = 0; j < symbol.parameterTypes.size(); ++j) {
                        if (j > 0) std::cout << ", ";
                        std::cout << symbol.parameterTypes[j];
                    }
                    std::cout << "]";
                }
                
                std::cout << std::endl;
            }
        }
    }

    std::vector<std::string> getUnusedVariables() const {
        std::vector<std::string> unused;
        for (const auto& scope : scopes) {
            for (const auto& [name, symbol] : scope) {
                if ((symbol.kind == SymbolKind::VARIABLE || symbol.kind == SymbolKind::FUNCTION) 
                    && !symbol.isUsed && !symbol.isBuiltin) {
                    unused.push_back(name);
                }
            }
        }
        return unused;
    }

private:
    Symbol* findSymbol(const std::string& name) {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) {
                return &found->second;
            }
        }
        return nullptr;
    }

    const Symbol* findSymbol(const std::string& name) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) {
                return &found->second;
            }
        }
        return nullptr;
    }

    // NEW: Parse function signature string
    void parseFunctionSignature(const std::string& signature, 
                                std::string& returnType, 
                                std::vector<std::string>& paramTypes) {
        size_t parenPos = signature.find('(');
        if (parenPos == std::string::npos) {
            returnType = signature;
            return;
        }
        
        returnType = signature.substr(0, parenPos);
        
        std::string paramStr = signature.substr(parenPos + 1);
        size_t endParen = paramStr.find(')');
        if (endParen != std::string::npos) {
            paramStr = paramStr.substr(0, endParen);
        }
        
        if (!paramStr.empty() && paramStr != "any") {
            // Split by comma
            size_t start = 0;
            size_t pos = 0;
            while ((pos = paramStr.find(',', start)) != std::string::npos) {
                paramTypes.push_back(paramStr.substr(start, pos - start));
                start = pos + 1;
            }
            paramTypes.push_back(paramStr.substr(start));
        }
    }

    std::string kindToString(SymbolKind kind) const {
        switch (kind) {
            case SymbolKind::VARIABLE: return "Variable";
            case SymbolKind::FUNCTION: return "Function";
            case SymbolKind::PARAMETER: return "Parameter";
            case SymbolKind::TEMPORARY: return "Temporary";
            default: return "Unknown";
        }
    }
};