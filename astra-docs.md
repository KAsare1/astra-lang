# Astra Programming Language Documentation

> **Version**: 1.0 Alpha  
> **Target Audience**: Systems programmers, compiler enthusiasts, and developers seeking type safety with performance

## Table of Contents

1. [Introduction](#introduction)
2. [Getting Started](#getting-started)
3. [Language Basics](#language-basics)
4. [Data Types](#data-types)
5. [Variables and Constants](#variables-and-constants)
6. [Operators](#operators)
7. [Expressions](#expressions)
8. [Built-in Functions](#built-in-functions)
9. [Error Handling](#error-handling)
10. [Compiler and Tooling](#compiler-and-tooling)
11. [Examples](#examples)
12. [Language Reference](#language-reference)
13. [Future Features](#future-features)

---

## Introduction

Astra is a statically-typed, compiled programming language designed for systems programming with an emphasis on memory safety, performance, and developer ergonomics. Built on LLVM infrastructure, Astra provides zero-cost abstractions while maintaining explicit control over system resources.

### Design Philosophy

- **Memory Safety**: Prevent common memory errors at compile time
- **Performance**: Zero-cost abstractions with predictable runtime behavior
- **Expressiveness**: Clean, readable syntax for complex operations
- **Interoperability**: Seamless integration with existing C libraries

### Key Features

- Static type system with type inference
- Compile-time memory safety checks
- LLVM-based code generation for optimal performance
- Cross-platform compatibility
- Comprehensive error messages with suggestions

---

## Getting Started

### Installation

```bash
# Clone the repository
git clone https://github.com/your-username/astra-lang
cd astra-lang

# Build the compiler
make all

# Verify installation
./astra --version
```

### Your First Program

Create a file named `hello.astra`:

```astra
let message = "Hello, Astra!";
print(message);
```

Compile and run:

```bash
./astra hello.astra
gcc -c src/code-generator/runtime.c -o build/runtime.o
gcc build/output.o build/runtime.o -o hello
./hello
```

Output:
```
Hello, Astra!
```

---

## Language Basics

### Syntax Overview

Astra uses a C-family syntax with modern language features:

```astra
// Single-line comments
/* Multi-line 
   comments */

// Variable declarations
let variable_name = value;

// Expression statements
expression;

// Function calls
print(value);
```

### Program Structure

An Astra program consists of a sequence of statements executed in order:

```astra
// Variable declarations
let x = 10;
let y = 20;

// Expressions and function calls
let sum = x + y;
print(sum);
```

---

## Data Types

### Primitive Types

| Type | Description | Example | Size |
|------|-------------|---------|------|
| `int` | 64-bit signed integer | `42`, `-17` | 8 bytes |
| `double` | 64-bit floating point | `3.14`, `-1.5` | 8 bytes |
| `string` | UTF-8 string literal | `"hello"` | Pointer |
| `char` | Single character | `'a'`, `'X'` | 1 byte |
| `bool` | Boolean value (represented as int) | `true`/`false` | 8 bytes |

### Type Inference

Astra automatically infers types from literal values:

```astra
let integer = 42;        // Type: int
let floating = 3.14;     // Type: double
let text = "Hello";      // Type: string
let character = 'A';     // Type: char
let flag = true;         // Type: int (boolean)
```

### Type Safety

Astra enforces strict type checking at compile time:

```astra
let x = 10;        // int
let y = 3.14;      // double
let invalid = x + y;  // ❌ Compile error: Type mismatch
```

---

## Variables and Constants

### Variable Declaration

Variables are declared using the `let` keyword:

```astra
let variable_name = initial_value;
```

#### Examples

```astra
// Basic declarations
let count = 0;
let rate = 0.05;
let name = "Astra";

// Type is inferred from the initializer
let temperature = 98.6;  // double
let is_valid = count > 0;  // int (boolean result)
```

### Variable Scope

Variables follow lexical scoping rules:

```astra
let global_var = 100;

// Variables declared in inner scopes shadow outer ones
let x = 10;
// Future: Block scopes will be supported
```

### Initialization

All variables must be initialized at declaration:

```astra
let uninitialized;  // ❌ Compile error: Missing initializer
let initialized = 0;  // ✅ Valid
```

---

## Operators

### Arithmetic Operators

| Operator | Description | Example | Result |
|----------|-------------|---------|--------|
| `+` | Addition | `5 + 3` | `8` |
| `-` | Subtraction | `5 - 3` | `2` |
| `*` | Multiplication | `5 * 3` | `15` |
| `/` | Division | `6 / 3` | `2` |
| `%` | Modulo (integers only) | `7 % 3` | `1` |

#### Type-Specific Behavior

```astra
// Integer arithmetic
let int_result = 7 / 3;      // Result: 2 (integer division)

// Floating-point arithmetic  
let float_result = 7.0 / 3.0;  // Result: 2.333...

// Modulo only works with integers
let mod_result = 7 % 3;      // Result: 1
let invalid = 7.0 % 3.0;     // ❌ Compile error
```

### Comparison Operators

| Operator | Description | Example | Result Type |
|----------|-------------|---------|-------------|
| `==` | Equal to | `5 == 5` | `int` (boolean) |
| `!=` | Not equal to | `5 != 3` | `int` (boolean) |
| `<` | Less than | `3 < 5` | `int` (boolean) |
| `<=` | Less than or equal | `3 <= 5` | `int` (boolean) |
| `>` | Greater than | `5 > 3` | `int` (boolean) |
| `>=` | Greater than or equal | `5 >= 3` | `int` (boolean) |

```astra
let a = 10;
let b = 5;

let is_equal = a == b;      // 0 (false)
let is_greater = a > b;     // 1 (true)
let is_different = a != b;  // 1 (true)
```

### Logical Operators

| Operator | Description | Example | Behavior |
|----------|-------------|---------|----------|
| `&&` | Logical AND | `true && false` | Short-circuit evaluation |
| `\|\|` | Logical OR | `true \|\| false` | Short-circuit evaluation |
| `!` | Logical NOT | `!true` | Unary negation |

```astra
let x = 10;
let y = 5;

let both_true = (x > 0) && (y > 0);    // 1 (true)
let either_true = (x < 0) || (y > 0);  // 1 (true)
let negated = !(x > y);                // 0 (false)
```

### Unary Operators

| Operator | Description | Example | Notes |
|----------|-------------|---------|-------|
| `-` | Unary minus | `-x` | Numeric negation |
| `!` | Logical NOT | `!flag` | Boolean negation |

```astra
let positive = 42;
let negative = -positive;  // -42

let flag = 1;
let not_flag = !flag;     // 0
```

### Operator Precedence

From highest to lowest precedence:

1. **Unary operators**: `-`, `!`
2. **Multiplicative**: `*`, `/`, `%`
3. **Additive**: `+`, `-`
4. **Comparison**: `<`, `<=`, `>`, `>=`
5. **Equality**: `==`, `!=`
6. **Logical AND**: `&&`
7. **Logical OR**: `||`

```astra
// Precedence example
let result = 2 + 3 * 4;        // 14, not 20
let grouped = (2 + 3) * 4;     // 20
let complex = -2 + 3 * 4 > 10; // 1 (true)
```

---

## Expressions

### Simple Expressions

```astra
let literal = 42;           // Literal expression
let variable = literal;     // Variable expression
let arithmetic = 10 + 5;    // Binary expression
let negated = -arithmetic;  // Unary expression
```

### Complex Expressions

```astra
let a = 10;
let b = 5;
let c = 2;

// Complex arithmetic with precedence
let result = a + b * c - (a / b);  // 10 + 10 - 2 = 18

// Chained comparisons
let in_range = (0 < a) && (a < 100);  // 1 (true)

// Mixed operations
let complex = (a > b) && ((a % c) == 0);  // 1 (true)
```

### Parentheses and Grouping

Use parentheses to override operator precedence:

```astra
let without_parens = 2 + 3 * 4;    // 14
let with_parens = (2 + 3) * 4;     // 20

let logical = a > 0 && b > 0 || c < 0;     // (a > 0 && b > 0) || c < 0
let grouped = a > 0 && (b > 0 || c < 0);   // a > 0 && (b > 0 || c < 0)
```

---

## Built-in Functions

### Print Function

The `print()` function outputs values to standard output:

```astra
print(42);           // Outputs: 42
print(3.14);         // Outputs: 3.14
print("Hello");      // Outputs: Hello
```

#### Type Overloading

The print function automatically selects the appropriate implementation based on argument type:

- `print_i64()` for integers
- `print_double()` for floating-point numbers
- `print_str()` for strings

```astra
let integer = 100;
let floating = 3.14159;
let message = "Debug info";

print(integer);   // Calls print_i64()
print(floating);  // Calls print_double()
print(message);   // Calls print_str()
```

---

## Error Handling

### Compile-Time Errors

Astra provides comprehensive error messages with helpful suggestions:

#### Type Mismatch Error

```astra
let x = 10;
let y = 3.14;
let invalid = x + y;
```

**Output:**
```
error: [semantic] Type mismatch in binary expression: 'int' + 'double'
  suggestion: Ensure both operands have the same type
  suggestion: Consider using explicit type conversion
```

#### Undeclared Variable Error

```astra
print(undeclared_var);
```

**Output:**
```
error: [semantic] Use of undeclared variable 'undeclared_var'
  suggestion: Declare the variable before using it
  suggestion: Check for typos in variable name
```

### Error Categories

- **Lexical**: Tokenization errors (invalid characters, unterminated strings)
- **Syntax**: Grammar violations (missing semicolons, unmatched parentheses)
- **Semantic**: Type errors, undeclared variables, scope violations
- **Codegen**: IR generation failures
- **System**: File I/O errors, linking failures

### Warnings

The compiler provides helpful warnings for potential issues:

```astra
let unused_variable = 42;
// Warning: Variable 'unused_variable' declared but never used
```

---

## Compiler and Tooling

### Command Line Interface

```bash
# Basic compilation
./astra source_file.astra

# Verbose output showing all phases
./astra --verbose source_file.astra

# Output to specific files
./astra -o output_name source_file.astra
```

### Build Process

The Astra compiler follows a multi-phase compilation process:

1. **Lexical Analysis**: Source code → Tokens
2. **Syntax Analysis**: Tokens → Abstract Syntax Tree (AST)
3. **Semantic Analysis**: Type checking and symbol resolution
4. **IR Generation**: AST → LLVM Intermediate Representation
5. **Target Generation**: LLVM IR → Assembly/Object code
6. **Linking**: Object code + Runtime → Executable

### Generated Files

| File | Description | Purpose |
|------|-------------|---------|
| `output.ll` | LLVM IR | Human-readable intermediate code |
| `output.s` | Assembly | Platform-specific assembly code |
| `output.o` | Object file | Compiled binary object |

### Integration with Build Systems

#### Makefile Integration

```makefile
# Compile Astra source files
%.o: %.astra
	./astra $<
	gcc -c src/code-generator/runtime.c -o build/runtime.o
	gcc build/output.o build/runtime.o -o $@
```

---

## Examples

### Basic Arithmetic Calculator

```astra
// Simple calculator demonstrating all arithmetic operations
let a = 15;
let b = 4;

print(a + b);  // Addition: 19
print(a - b);  // Subtraction: 11
print(a * b);  // Multiplication: 60
print(a / b);  // Division: 3
print(a % b);  // Modulo: 3
```

### Conditional Logic Simulation

```astra
// Simulating conditional logic with boolean operations
let temperature = 75;
let humidity = 60;
let is_comfortable = (temperature >= 70) && (temperature <= 80) && (humidity <= 65);

print(is_comfortable);  // Outputs: 1 (true)
```

### Mathematical Expressions

```astra
// Complex mathematical calculations
let radius = 5.0;
let pi = 3.14159;

let circumference = 2.0 * pi * radius;
let area = pi * radius * radius;

print(circumference);  // Outputs: 31.4159
print(area);          // Outputs: 78.5398
```

### Order of Operations

```astra
// Demonstrating operator precedence
let result1 = 2 + 3 * 4;          // 14 (not 20)
let result2 = (2 + 3) * 4;        // 20
let result3 = 2 * 3 + 4 * 5;      // 26
let result4 = 2 * (3 + 4) * 5;    // 70

print(result1);
print(result2);  
print(result3);
print(result4);
```

### Type Safety Demonstration

```astra
// This program shows type safety in action
let integer_value = 42;
let float_value = 3.14;

// Valid operations (same types)
let int_sum = integer_value + 10;        // int + int = int
let float_sum = float_value + 2.86;      // double + double = double

print(int_sum);    // 52
print(float_sum);  // 6.0

// The following would cause compile errors:
// let invalid = integer_value + float_value;  // Type mismatch!
// let also_invalid = float_value % 2.0;       // Modulo on floats!
```

---

## Language Reference

### Keywords

#### Currently Implemented
- `let` - Variable declaration

#### Reserved for Future Use
- `fn` - Function definition
- `struct` - Structure definition
- `copy`, `unique`, `shared` - Ownership types
- `ref`, `mutref` - Reference types
- `match`, `case` - Pattern matching
- `unsafe` - Unsafe code blocks
- `extern` - Foreign function interface
- `drop`, `defer` - Resource management
- `return` - Function return
- `spawn` - Concurrency

### Operators Reference

#### Arithmetic
```astra
+  -  *  /  %    // Binary arithmetic
-  !             // Unary operators
```

#### Comparison
```astra
==  !=  <  <=  >  >=
```

#### Logical
```astra
&&  ||  !
```

#### Assignment
```astra
=               // Assignment (in declarations)
```

#### Punctuation
```astra
( )             // Grouping
{ }             // Future: Block scope
[ ]             // Future: Arrays/indexing
, ; :           // Separators
-> =>           // Future: Function syntax
```

### Grammar (EBNF)

```ebnf
program        = statement* EOF ;
statement      = varDecl | exprStmt ;
varDecl        = "let" IDENTIFIER "=" expression ";" ;
exprStmt       = expression ";" ;

expression     = assignment ;
assignment     = logicalOr ;
logicalOr      = logicalAnd ( "||" logicalAnd )* ;
logicalAnd     = equality ( "&&" equality )* ;
equality       = comparison ( ( "!=" | "==" ) comparison )* ;
comparison     = additive ( ( ">" | ">=" | "<" | "<=" ) additive )* ;
additive       = multiplicative ( ( "-" | "+" ) multiplicative )* ;
multiplicative = unary ( ( "/" | "*" | "%" ) unary )* ;
unary          = ( "!" | "-" ) unary | call ;
call           = IDENTIFIER ( "(" arguments? ")" )? | primary ;
primary        = "true" | "false" | NUMBER | STRING | CHAR 
               | IDENTIFIER | "(" expression ")" ;

arguments      = expression ( "," expression )* ;
```

---

## Future Features

### Planned Language Features

#### Functions
```astra
// Function definition (planned)
fn calculate_area(radius: double) -> double {
    let pi = 3.14159;
    return pi * radius * radius;
}

// Function call
let area = calculate_area(5.0);
```

#### Structures
```astra
// Structure definition (planned)
struct Point {
    x: double,
    y: double,
}

// Structure usage
let origin = Point { x: 0.0, y: 0.0 };
```

#### Control Flow
```astra
// Conditional statements (planned)
if condition {
    // true branch
} else {
    // false branch
}

// Loops (planned)
while condition {
    // loop body
}

for i in 0..10 {
    // iteration
}
```

#### Pattern Matching
```astra
// Pattern matching (planned)
match value {
    case 0 => print("zero"),
    case 1..10 => print("small"),
    case _ => print("large"),
}
```

#### Memory Management
```astra
// Ownership types (planned)
let owned: unique<String> = "Hello";
let borrowed: ref<String> = &owned;
let shared_data: shared<Vector> = make_shared();
```

#### Arrays and Collections
```astra
// Arrays (planned)
let numbers: [int; 5] = [1, 2, 3, 4, 5];
let dynamic: Vec<int> = vec![1, 2, 3];
```

### Standard Library (Planned)

- **I/O**: File reading/writing, formatted output
- **Collections**: Vectors, maps, sets
- **String manipulation**: Parsing, formatting, regex
- **Math**: Advanced mathematical functions
- **Concurrency**: Threads, async/await, channels
- **Networking**: HTTP client/server, sockets
- **System**: File system operations, process management

### Tooling Roadmap

- **Package manager**: Dependency management and distribution
- **Language server**: IDE integration with autocomplete and diagnostics
- **Debugger**: Integration with GDB/LLDB
- **Profiler**: Performance analysis tools
- **Documentation generator**: Automatic API documentation
- **Test framework**: Unit testing and benchmarking

---

## Contributing

### Development Setup

```bash
# Clone the repository
git clone https://github.com/your-username/astra-lang
cd astra-lang

# Install dependencies (LLVM, GCC)
# On macOS:
brew install llvm gcc

# On Ubuntu:
sudo apt install llvm-dev gcc

# Build the compiler
make clean && make all

# Run tests
make test
```

### Code Style

- Follow C++17 standards
- Use meaningful variable and function names
- Add comprehensive error handling
- Include unit tests for new features
- Document public APIs

### Reporting Issues

Please report bugs and feature requests on our GitHub repository with:

- Minimal reproduction case
- Expected vs. actual behavior
- System information (OS, compiler version)
- Astra version

---

*This documentation reflects Astra Language Version 1.0 Alpha. Features marked as "planned" or "future" are not yet implemented but are part of the language roadmap.*