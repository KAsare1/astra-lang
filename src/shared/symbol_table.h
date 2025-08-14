#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include <stdexcept>

struct Symbol {
    std::string name;
    std::string type; // e.g., "int", "float", "string"
};

class SymbolTable {
    std::vector<std::unordered_map<std::string, Symbol>> scopes;

public:
    SymbolTable() {
        enterScope(); // Start with global scope
    }

    // Enters a new scope
    void enterScope() {
        scopes.emplace_back();
    }

    // Exits the current scope
    void exitScope() {
        if (scopes.empty()) {
            throw std::runtime_error("No scope to exit.");
        }
        scopes.pop_back();
    }

    // Adds a new symbol to the current scope
    void declare(const std::string& name) {
        if (scopes.empty()) {
            throw std::runtime_error("No scope available to declare symbol: " + name);
        }
        scopes.back()[name] = Symbol{name};
    }

    // Checks if a symbol is declared in any scope
    bool isDeclared(const std::string& name) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            if (it->find(name) != it->end()) {
                return true;
            }
        }
        return false;
    }

    void setType(const std::string& name, const std::string& type) {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            if (it->find(name) != it->end()) {
                it->at(name).type = type;
                return;
            }
        }
        throw std::runtime_error("Symbol not found: " + name);
    }

    std::string getType(const std::string& name) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            if (it->find(name) != it->end()) {
                return it->at(name).type;
            }
        }
        throw std::runtime_error("Symbol not found: " + name);
    }
};
