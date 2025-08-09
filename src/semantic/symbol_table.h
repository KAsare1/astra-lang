#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include <stdexcept>

struct Symbol {
    std::string name;
    // In future: type, const/let flag, etc.
};

class SymbolTable {
    std::vector<std::unordered_map<std::string, Symbol>> scopes;

public:
    SymbolTable() {
        enterScope(); // Start with global scope
    }

    void enterScope() {
        scopes.push_back({});
    }

    void exitScope() {
        if (scopes.empty()) {
            throw std::runtime_error("No scope to exit.");
        }
        scopes.pop_back();
    }

    void declare(const std::string& name) {
        auto& current = scopes.back();
        if (current.find(name) != current.end()) {
            throw std::runtime_error("Variable '" + name + "' redeclared in the same scope.");
        }
        current[name] = Symbol{name};
    }

    bool isDeclared(const std::string& name) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            if (it->find(name) != it->end()) {
                return true;
            }
        }
        return false;
    }
};
