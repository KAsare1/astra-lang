# astra-lang Language Specification

## Overview
**Astra** is a new programming language currently under active development.
This document describes the *current syntax* as supported by the lexer and parser at this stage of the project.

*> ⚠ ***Note***: The grammar is minimal and will expand over time as the compiler gains more features.*

---

## Project Structure
```
├── build/ # Build directory (CMake/Make artifacts)
│ ├── lexer/ # Lexer build artifacts
│ ├── parser/ # Parser build artifacts
│ └── main.o
├── astra # Final compiled executable (CMake)
├── compiler # Final compiled executable (Make)
├── examples/ # Example Astra programs
├── CMakeLists.txt # CMake build configuration
├── Makefile # Make build automation
├── src/
│ ├── abstract-syntax-tree/ # AST node definitions
│ ├── code-generator/ # Code generation
│ ├── lexer/ # Lexer implementation
│ ├── parser/ # Parser implementation
│ ├── semantic/ # (Planned) Semantic analysis
│ └── main.cpp # Compiler entry point
└── tests/
    └── astra.acc # Test program
```

## Building the Compiler

The project supports both **CMake** and **Make** build systems.

### Option 1: CMake (Recommended)

CMake provides better dependency management and cross-platform support:

```bash
# Configure the build (only needed once or when CMakeLists.txt changes)
cmake -B build

# Build the compiler
cmake --build build

# The executable will be created as ./build/astra
```

To clean and rebuild:
```bash
# Clean build directory
rm -rf build

# Reconfigure and build
cmake -B build
cmake --build build
```

### Option 2: Traditional Make

This will produce the compiler executable as `./compiler` in the root directory:

```bash
make
```

To clean and rebuild:
```bash
make clean && make
```

## Running the Compiler on Sample Code

### With CMake build:
```bash
./build/astra tests/astra.acc
```

### With Make build:
```bash
./compiler tests/astra.acc
```

## Modifying the Compiler

The compiler is modular, so changes are split into different folders:

**Lexer (src/lexer/)**
Handles breaking source code into tokens.
Edit lexer.cpp and keywords.h if you want to add new keywords or symbols.

**Parser (src/parser/)**
Converts tokens into an Abstract Syntax Tree (AST).
Modify parser.cpp and parser.h to support new grammar rules.

**Abstract Syntax Tree (src/abstract-syntax-tree/)**
Data structures representing the parsed code.
Update ast.h and ast_printer.h when adding new node types.

**Semantic Analysis (src/semantic/)**
(Planned) This stage will handle type checking and scope validation.

**Code Generator (src/code-generator/)**
Converts AST into executable code or bytecode using LLVM.

After making changes, rebuild the compiler:

### With CMake:
```bash
cmake --build build
```

### With Make:
```bash
make clean && make
```

---

## Grammar
The syntax is described in **EBNF** form:
```
program ::= { declaration } EOF
declaration ::= var_declaration
             | statement
var_declaration ::= "let" IDENTIFIER [ "=" expression ]
statement ::= expression_statement
expression_statement ::= expression
expression ::= primary
primary ::= literal
          | IDENTIFIER
literal ::= INT_LITERAL
          | FLOAT_LITERAL
          | STRING_LITERAL
```

---

## Tokens

### Keywords
```
let
```

### Literals
* **Integer literals** — e.g., `42`, `100`
* **Float literals** — e.g., `3.14`, `0.5`
* **String literals** — `"hello"`

### Identifiers
* Names for variables and functions: must start with a letter or underscore, followed by letters, digits, or underscores.

### Symbols
```
= ( )
```

---

## Example Code
```astra
let x = 42
print(x)
```

## Current Limitations
* No operators or complex expressions yet (binary expressions not parsed).
* No control flow statements.
* No functions, structs, or imports yet.
* No type annotations.
* Only single-assignment variable declarations are supported.

---

## Next Steps
Planned future syntax features:
* Arithmetic and logical operators
* Function definitions and calls
* Conditional and loop constructs
* Structs and custom types
* Type inference and annotations
* Pattern matching