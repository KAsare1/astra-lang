# astra-lang Language Specification

## Overview
**Astra** is a new programming language currently under active development.  
This document describes the *current syntax* as supported by the lexer and parser at this stage of the project.

> ⚠ **Note**: The grammar is minimal and will expand over time as the compiler gains more features.

---

## Project Structure
```

├── build/ # Compiled object files
│ ├── lexer/ # Lexer build artifacts
│ ├── parser/ # Parser build artifacts
│ └── main.o
├── compiler # Final compiled executable
├── examples/ # Example Astreon programs
├── Makefile # Build automation
├── src/
│ ├── abstract-syntax-tree/ # AST node definitions
│ ├── code-generator/ # (Planned) Code generation
│ ├── lexer/ # Lexer implementation
│ ├── parser/ # Parser implementation
│ ├── semantic/ # (Planned) Semantic analysis
│ └── main.cpp # Compiler entry point
└── tests/
└── astra.acc # Test program

```

## Building the compiler
This project uses `make` for building.

This will produce the compiler executable in the root directory.

```bash
make
```

## Running the compiler on sample code
```bash
 ./astra tests/astra.acc
```


## Modifying the Compiler
The compiler is modular, so changes are split into different folders:

Lexer (src/lexer/)
Handles breaking source code into tokens.
Edit lexer.cpp and keywords.h if you want to add new keywords or symbols.

Parser (src/parser/)
Converts tokens into an Abstract Syntax Tree (AST).
Modify parser.cpp and parser.h to support new grammar rules.

Abstract Syntax Tree (src/abstract-syntax-tree/)
Data structures representing the parsed code.
Update ast.h and ast_printer.h when adding new node types.

Semantic Analysis (src/semantic/)
(Planned) This stage will handle type checking and scope validation.

Code Generator (src/code-generator/)
(Planned) Will convert AST into executable code or bytecode.

After making changes, rebuild the compiler:

bash
```
make clean && make
```

---

## Grammar

The syntax is described in **EBNF** form:

```
program        ::= { declaration } EOF

declaration    ::= var_declaration
                 | statement

var_declaration ::= "let" IDENTIFIER [ "=" expression ]

statement      ::= expression_statement

expression_statement ::= expression

expression     ::= primary

primary        ::= literal
                 | IDENTIFIER

literal        ::= INT_LITERAL
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
=   (   )
```

---


## Example Code

```astra
let x = 42
print(x)
````


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

