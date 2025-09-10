# Astra-lang Compiler

## Overview
**Astra** is a modern programming language compiler built with LLVM. The project demonstrates a complete compilation pipeline from lexical analysis through native code generation, featuring professional error handling, comprehensive symbol table management, and extensible architecture.

## Current Implementation Status

###  Fully Implemented Features
- **Complete Compilation Pipeline**: Source code → Tokens → AST → Semantic Analysis → LLVM IR → Native Assembly → Executable
- **Professional Error Handling**: Context-aware error messages with line numbers, suggestions, and recovery
- **Shared Symbol Table**: Consistent symbol management across all compiler phases
- **LLVM Integration**: Full IR generation and native code output
- **Type System Foundation**: Basic type inference for integers, floats, and strings
- **Built-in Functions**: Polymorphic `print()` function with type-specific implementations

### Current Language Capabilities
```astra
let x = 42;
let pi = 3.14;
let message = "Hello, World!";
print(x);
print(pi);
print(message);
```

---

## Project Architecture

```
astra-cc/
├── build/                    # Build artifacts and output files
│   ├── output.ll            # Generated LLVM IR
│   ├── output.s             # Generated assembly
│   ├── output.o             # Generated object file
│   └── runtime.o            # Runtime library
├── src/
│   ├── shared/              # Shared components
│   │   ├── symbol_table.h   # Symbol table with scope management
│   │   └── error_handler.h  # Professional error reporting
│   ├── lexer/               # Lexical analysis
│   │   ├── lexer.h/.cpp     # Tokenizer implementation
│   │   ├── token.h          # Token definitions
│   │   └── keywords.h       # Language keywords
│   ├── parser/              # Syntax analysis
│   │   ├── parser.h/.cpp    # Parser with error recovery
│   │   └── ast.h            # AST node definitions
│   ├── semantic/            # Semantic analysis
│   │   └── semantic_analyzer.h # Type checking and validation
│   ├── code-generator/      # Code generation
│   │   ├── ir_codegen.h/.cpp    # LLVM IR generation
│   │   ├── target/              # Target-specific code
│   │   └── runtime.c            # Runtime support functions
│   └── main.cpp             # Compiler driver
├── tests/
│   └── astra.acc            # Test programs
├── CMakeLists.txt           # CMake configuration
└── Makefile                 # Alternative build system
```

## Building the Compiler

### Prerequisites
- **LLVM 15+** with development headers
- **CMake 3.16+** or **Make**
- **GCC/Clang** with C++17 support

### Installation (macOS with Homebrew)
```bash
brew install llvm cmake
```

### Build Options

#### Option 1: CMake (Recommended)
```bash
# Configure and build
mkdir build && cd build
cmake ..
make

# Or use the simplified commands:
cmake -B build
cmake --build build
```

#### Option 2: Traditional Make
```bash
make clean && make
```

### Running the Compiler
```bash
# With CMake build
./build/astra tests/astra.acc

# With Make build  
./astra tests/astra.acc
```

### Complete Compilation Process
```bash
# 1. Compile Astra source to object file
./astra program.astra

# 2. Link with runtime to create executable
gcc -c src/code-generator/runtime.c -o build/runtime.o
gcc build/output.o build/runtime.o -o my_program

# 3. Run your program
./my_program
```

---

## Compiler Architecture Deep Dive

### Phase 1: Lexical Analysis (`src/lexer/`)
- **Tokenizes** source code into meaningful symbols
- **Tracks** line/column information for error reporting
- **Handles** string literals, numbers, identifiers, and keywords
- **Reports** lexical errors (unterminated strings, invalid characters)

### Phase 2: Syntax Analysis (`src/parser/`)
- **Builds** Abstract Syntax Tree from token stream
- **Implements** recursive descent parsing with error recovery
- **Manages** symbol declarations and scope validation
- **Provides** synchronization points for multiple error reporting

### Phase 3: Semantic Analysis (`src/semantic/`)
- **Performs** type inference and checking
- **Validates** variable usage and function calls
- **Detects** unused variables and uninitialized usage
- **Maintains** symbol table with type information

### Phase 4: Code Generation (`src/code-generator/`)
- **Generates** LLVM IR from AST
- **Produces** native assembly and object code
- **Links** with runtime library for built-in functions
- **Optimizes** through LLVM's optimization passes

### Shared Components (`src/shared/`)
- **Symbol Table**: Multi-scope symbol management with type tracking
- **Error Handler**: Professional error reporting with context and suggestions

---

## Adding New Language Features

### 1. Adding New Operators (e.g., Arithmetic)

#### Step 1: Update Lexer
```cpp
// In lexer/keywords.h - add new token types
enum class TokenType {
    // ... existing tokens ...
    PLUS, MINUS, MULTIPLY, DIVIDE
};

// In lexer/lexer.cpp - recognize new symbols
void Lexer::symbol() {
    char c = advance();
    switch (c) {
        case '+': addToken(TokenType::PLUS, "+"); break;
        case '-': addToken(TokenType::MINUS, "-"); break;
        // ... etc
    }
}
```

#### Step 2: Update AST
```cpp
// In abstract-syntax-tree/ast.h
struct BinaryExpr : Expr {
    std::unique_ptr<Expr> left;
    std::string op;
    std::unique_ptr<Expr> right;
    // ... constructor
};
```

#### Step 3: Update Parser
```cpp
// In parser/parser.cpp - add precedence parsing
std::unique_ptr<Expr> Parser::expression() {
    return additive();
}

std::unique_ptr<Expr> Parser::additive() {
    auto expr = multiplicative();
    while (match({TokenType::PLUS, TokenType::MINUS})) {
        std::string op = previous().lexeme;
        auto right = multiplicative();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}
```

#### Step 4: Update Semantic Analyzer
```cpp
// In semantic/semantic_analyzer.h
std::string analyzeExpr(const Expr* expr) {
    // ... existing cases ...
    else if (auto binary = dynamic_cast<const BinaryExpr*>(expr)) {
        std::string leftType = analyzeExpr(binary->left.get());
        std::string rightType = analyzeExpr(binary->right.get());
        
        if (leftType != rightType) {
            errorHandler.reportError(ErrorCategory::SEMANTIC,
                "Type mismatch in binary expression");
            return "error";
        }
        return leftType;
    }
}
```

#### Step 5: Update Code Generator
```cpp
// In code-generator/ir_codegen.cpp
llvm::Value* IRCodegen::genBinary(const BinaryExpr* bin) {
    llvm::Value* left = genExpr(bin->left.get());
    llvm::Value* right = genExpr(bin->right.get());
    
    if (bin->op == "+") {
        if (left->getType()->isIntegerTy()) {
            return builder.CreateAdd(left, right, "addtmp");
        } else if (left->getType()->isDoubleTy()) {
            return builder.CreateFAdd(left, right, "addtmp");
        }
    }
    // ... handle other operators
}
```

### 2. Adding Control Flow (e.g., If Statements)

#### Follow the same pattern:
1. **Add tokens** for `if`, `else`, `{`, `}`
2. **Create AST nodes** for `IfStmt`  
3. **Parse** control flow syntax
4. **Validate** condition types in semantic analysis
5. **Generate** conditional branches in LLVM IR

### 3. Adding Functions

This requires more extensive changes:
1. **Function declaration parsing**
2. **Parameter and return type handling**
3. **Call stack management**
4. **LLVM function generation**

---

## Error Handling System

The compiler features professional error handling with:

### Error Categories
- **Lexical**: Tokenization errors (unterminated strings)
- **Syntax**: Parsing errors (missing semicolons)
- **Semantic**: Type and scope errors (undeclared variables)
- **Codegen**: IR generation errors

### Error Recovery
- Parser continues after errors to find multiple issues
- Synchronization points prevent cascading errors
- Null checks prevent crashes from malformed AST

### Example Output
```
program.astra:5:12: error: [semantic] Use of undeclared variable 'undeclared_var'
    print(undeclared_var);
           ^
  suggestion: Check the variable name for typos

program.astra:8:9: warning: [semantic] Variable 'unused' declared but never used
  suggestion: Remove the unused variable or use it in your code

Compilation failed with 1 error(s) and 1 warning(s)
```

---

## Testing Your Changes

### Create Test Files
```bash
# Create a test program
echo 'let x = 42; print(x);' > test.astra

# Test the full pipeline
./astra test.astra
gcc -c src/code-generator/runtime.c -o build/runtime.o
gcc build/output.o build/runtime.o -o test_program
./test_program
```

### Debug Output
The compiler provides detailed phase-by-phase output:
- Token counts from lexer
- Statement counts from parser  
- Symbol table state after semantic analysis
- Generated file locations

---

## Contributing Guidelines

### Code Style
- Use modern C++17 features
- Follow RAII principles
- Prefer smart pointers over raw pointers
- Include comprehensive error checking

### Adding Features
1. **Design** the feature syntax and semantics
2. **Update** all relevant compiler phases
3. **Add** comprehensive error handling
4. **Test** with both valid and invalid code
5. **Document** the new functionality

### Common Pitfalls
- **Symbol table consistency**: Ensure all phases use the shared symbol table
- **Error propagation**: Handle null AST nodes from parsing errors
- **LLVM type matching**: Verify type consistency in IR generation
- **Memory management**: Use RAII and smart pointers consistently

---

## Current Limitations

### Language Features Not Yet Implemented
- Arithmetic and logical operators
- Control flow (if/else, loops)
- Function definitions
- User-defined types/structs
- Arrays and collections
- Module system

### Known Issues
- Binary expressions parsing is stubbed out
- Limited type system (no user-defined types)
- No optimization passes enabled
- Runtime library is minimal

---

## Future Roadmap

### Short Term
- [ ] Arithmetic operators (+, -, *, /)
- [ ] Boolean operators (&&, ||, !)
- [ ] Comparison operators (==, !=, <, >)
- [ ] Control flow (if/else, while)

### Medium Term  
- [ ] Function definitions and calls
- [ ] Arrays and basic collections
- [ ] Struct types
- [ ] Pattern matching

### Long Term
- [ ] Memory management features
- [ ] Concurrency primitives  
- [ ] Module system
- [ ] Standard library

The compiler architecture is designed to support these extensions naturally through the existing phase structure and symbol table system.





Recommended Implementation Order
Phase 1: Control Flow (Weeks 1-2)

Block statements with proper scoping
If/else statements
While loops
Break/continue (if desired)

Phase 2: Functions (Weeks 3-4)

Function definitions with parameters and return types
Return statements
Function calls with argument type checking
Local variable scoping within functions

Phase 3: Data Structures (Weeks 5-6)

Arrays with literal syntax and indexing
Enhanced string operations
Basic built-in functions (len, etc.)

Phase 4: Advanced Features (Weeks 7+)

Structures/records
Type annotations
Pattern matching (if desired)
Standard library expansion