# Enhanced Makefile for Astra Compiler with Data Structures Support

CC=gcc
CXX=g++
CFLAGS=-Wall -std=c11
CXXFLAGS=-Wall -std=c++17 -Isrc $(shell $(LLVM_DIR)/bin/llvm-config --cxxflags | sed 's/-fno-exceptions//g')

# Directories
SRC_DIR=src
BUILD_DIR=build
TARGET=astra
LLVM_DIR=/opt/homebrew/opt/llvm

# LLVM linking
LDFLAGS=-L$(LLVM_DIR)/lib -Wl,-rpath,$(LLVM_DIR)/lib
LDLIBS=$(shell $(LLVM_DIR)/bin/llvm-config --libs all) $(shell $(LLVM_DIR)/bin/llvm-config --system-libs)

# Source files
C_SRCS=$(shell find $(SRC_DIR) -name '*.c')
CXX_SRCS=$(shell find $(SRC_DIR) -name '*.cpp')

# Object files
C_OBJS=$(C_SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
CXX_OBJS=$(CXX_SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
OBJS=$(C_OBJS) $(CXX_OBJS)

# NEW: Runtime library components
RUNTIME_DIR=$(SRC_DIR)/code-generator
RUNTIME_SRCS = $(RUNTIME_DIR)/runtime.c \
               $(RUNTIME_DIR)/runtime_arrays.c \
               $(RUNTIME_DIR)/runtime_strings.c \
               $(RUNTIME_DIR)/runtime_builtins.c

RUNTIME_OBJS = $(BUILD_DIR)/code-generator/runtime.o \
               $(BUILD_DIR)/code-generator/runtime_arrays.o \
               $(BUILD_DIR)/code-generator/runtime_strings.o \
               $(BUILD_DIR)/code-generator/runtime_builtins.o

RUNTIME_LIB = $(BUILD_DIR)/libastra_runtime.a

# ===========================================
# COMPILATION RULES
# ===========================================

# C source compilation
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# C++ source compilation  
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ===========================================
# RUNTIME LIBRARY CREATION
# ===========================================

# Create runtime library
$(RUNTIME_LIB): $(RUNTIME_OBJS)
	@echo "Creating runtime library..."
	ar rcs $@ $(RUNTIME_OBJS)
	@echo "✅ Runtime library created: $@"

# Runtime library info
runtime-info: $(RUNTIME_LIB)
	@echo "Runtime library symbols:"
	nm $(RUNTIME_LIB) | grep -E "(string_|array_|print_|len_|push_|pop_)" | head -20
	@echo "... (showing first 20 symbols)"

# ===========================================
# MAIN TARGETS
# ===========================================

# Main compiler target
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS)
	@echo "✅ Compiler built successfully"

# Build everything
all: $(TARGET) $(RUNTIME_LIB)
	@echo "✅ Full build complete"

# Clean all build artifacts
clean:
	rm -rf $(BUILD_DIR) $(TARGET) test_program
	@echo "✅ Clean complete"

# ===========================================
# TESTING TARGETS
# ===========================================

# Enhanced test with data structures support
test: $(TARGET) $(RUNTIME_LIB)
	@echo "=== Running Astra Compiler with Data Structures Support ==="
	./$(TARGET) tests/astra.acc
	@echo ""
	@echo "=== Linking with Enhanced Runtime ==="
	gcc build/output.o $(RUNTIME_LIB) -o test_program
	@echo ""
	@echo "=== Running Compiled Program ==="
	./test_program
	@echo ""
	@echo "=== Test Complete ==="

# Test basic array functionality
test-arrays: $(TARGET) $(RUNTIME_LIB)
	@echo "=== Testing Array Functionality ==="
	@echo 'let arr: [int] = [1, 2, 3]; push(arr, 4); println(len(arr));' > /tmp/test_arrays.acc
	./$(TARGET) /tmp/test_arrays.acc
	gcc build/output.o $(RUNTIME_LIB) -o test_program
	./test_program
	@rm -f /tmp/test_arrays.acc
	@echo "✅ Array test complete"

# Test basic string functionality  
test-strings: $(TARGET) $(RUNTIME_LIB)
	@echo "=== Testing String Functionality ==="
	@echo 'let msg: string = "Hello" + " " + "World"; println(msg); println(len(msg));' > /tmp/test_strings.acc
	./$(TARGET) /tmp/test_strings.acc
	gcc build/output.o $(RUNTIME_LIB) -o test_program
	./test_program
	@rm -f /tmp/test_strings.acc
	@echo "✅ String test complete"

# Test built-in functions
test-builtins: $(TARGET) $(RUNTIME_LIB)
	@echo "=== Testing Built-in Functions ==="
	@echo 'let num: int = to_int("42"); let str: string = to_string(num); println(str);' > /tmp/test_builtins.acc
	./$(TARGET) /tmp/test_builtins.acc
	gcc build/output.o $(RUNTIME_LIB) -o test_program  
	./test_program
	@rm -f /tmp/test_builtins.acc
	@echo "✅ Built-ins test complete"

# Comprehensive test suite
test-all: test test-arrays test-strings test-builtins
	@echo "✅ All tests completed successfully!"

# Verbose test that shows all intermediate files
test-verbose: $(TARGET) $(RUNTIME_LIB)
	@echo "=== Running Astra Compiler (Verbose Mode) ==="
	./$(TARGET) tests/astra.acc
	@echo ""
	@echo "=== Generated LLVM IR ==="
	@cat build/output.ll
	@echo ""
	@echo "=== Generated Assembly (first 50 lines) ==="
	@head -50 build/output.s
	@echo ""
	@echo "=== Runtime Library Contents ==="
	@echo "Runtime library size: $$(du -h $(RUNTIME_LIB) | cut -f1)"
	@echo "Number of symbols: $$(nm $(RUNTIME_LIB) | wc -l)"
	@echo ""
	@echo "=== Linking and Running ==="
	gcc build/output.o $(RUNTIME_LIB) -o test_program
	./test_program

# Test with different source files
test-file: $(TARGET) $(RUNTIME_LIB)
	@if [ -z "$(FILE)" ]; then \
		echo "Usage: make test-file FILE=path/to/your/file.acc"; \
		exit 1; \
	fi
	@echo "=== Testing file: $(FILE) ==="
	./$(TARGET) $(FILE)
	gcc build/output.o $(RUNTIME_LIB) -o test_program
	./test_program

# ===========================================
# DEVELOPMENT HELPERS
# ===========================================

# Compile only the runtime library (for development)
runtime: $(RUNTIME_LIB)
	@echo "✅ Runtime library compiled"

# Compile only the compiler (without runtime)
compiler: $(TARGET)
	@echo "✅ Compiler compiled"

# Show runtime library statistics
runtime-stats: $(RUNTIME_LIB)
	@echo "=== Runtime Library Statistics ==="
	@echo "File size: $$(du -h $(RUNTIME_LIB) | cut -f1)"
	@echo "Symbol count: $$(nm $(RUNTIME_LIB) | wc -l)"
	@echo ""
	@echo "Function categories:"
	@echo "  Arrays: $$(nm $(RUNTIME_LIB) | grep -c array_)"
	@echo "  Strings: $$(nm $(RUNTIME_LIB) | grep -c string_)"
	@echo "  Built-ins: $$(nm $(RUNTIME_LIB) | grep -cE '(len_|push_|pop_|builtin_)')"
	@echo "  I/O: $$(nm $(RUNTIME_LIB) | grep -cE '(print_|input_|println_)')"

# Lint runtime code
lint-runtime:
	@echo "=== Linting Runtime Code ==="
	@for file in $(RUNTIME_SRCS); do \
		echo "Checking $$file..."; \
		gcc -Wall -Wextra -std=c11 -fsyntax-only $$file -Isrc; \
	done
	@echo "✅ Runtime code linting complete"

# Format runtime code  
format-runtime:
	@echo "=== Formatting Runtime Code ==="
	@for file in $(RUNTIME_SRCS) $(RUNTIME_DIR)/runtime.h; do \
		echo "Formatting $$file..."; \
		clang-format -i $$file; \
	done
	@echo "✅ Runtime code formatting complete"

# Check for memory leaks (requires valgrind)
test-memory: $(TARGET) $(RUNTIME_LIB)
	@echo "=== Memory Leak Testing ==="
	@if ! command -v valgrind >/dev/null 2>&1; then \
		echo "❌ Valgrind not found. Install with: brew install valgrind"; \
		exit 1; \
	fi
	./$(TARGET) tests/astra.acc
	gcc build/output.o $(RUNTIME_LIB) -o test_program
	valgrind --leak-check=full --show-leak-kinds=all ./test_program
	@echo "✅ Memory test complete"

# ===========================================
# EXAMPLE PROGRAMS
# ===========================================

# Generate example programs for testing
examples:
	@mkdir -p examples
	@echo 'let numbers: [int] = [1, 2, 3, 4, 5]; for (i in 0:len(numbers)) { println(numbers[i]); }' > examples/arrays.acc
	@echo 'let greeting: string = "Hello"; let name: string = "World"; println(greeting + " " + name);' > examples/strings.acc
	@echo 'let matrix: [[int]] = [[1, 2], [3, 4]]; println(matrix[1][0]);' > examples/nested_arrays.acc
	@echo 'fn factorial(n: int) -> int { if (n <= 1) { return 1; } return n * factorial(n - 1); } println(factorial(5));' > examples/functions.acc
	@echo "✅ Example programs created in examples/"

# Test all examples
test-examples: $(TARGET) $(RUNTIME_LIB) examples
	@echo "=== Testing Example Programs ==="
	@for example in examples/*.acc; do \
		echo "Testing $$example..."; \
		./$(TARGET) $$example && gcc build/output.o $(RUNTIME_LIB) -o test_program && ./test_program; \
		echo ""; \
	done
	@echo "✅ All examples tested"

# ===========================================
# DEBUGGING AND ANALYSIS
# ===========================================

# Debug build with additional flags
debug: CFLAGS += -g -DDEBUG -O0
debug: CXXFLAGS += -g -DDEBUG -O0  
debug: $(TARGET) $(RUNTIME_LIB)
	@echo "✅ Debug build complete"

# Release build with optimizations
release: CFLAGS += -O3 -DNDEBUG
release: CXXFLAGS += -O3 -DNDEBUG
release: $(TARGET) $(RUNTIME_LIB)
	@echo "✅ Release build complete"

# Clean up all generated files
test-clean:
	rm -f test_program build/runtime.o build/output.* /tmp/test_*.acc
	@echo "✅ Test artifacts cleaned"

# Show help
help:
	@echo "Available targets:"
	@echo "  all           - Build compiler and runtime library"
	@echo "  compiler      - Build compiler only"
	@echo "  runtime       - Build runtime library only"
	@echo "  test          - Run basic functionality test"
	@echo "  test-arrays   - Test array functionality"
	@echo "  test-strings  - Test string functionality"
	@echo "  test-builtins - Test built-in functions"
	@echo "  test-all      - Run comprehensive test suite"
	@echo "  test-verbose  - Run test with detailed output"
	@echo "  test-file     - Test specific file (FILE=path)"
	@echo "  test-memory   - Run memory leak tests"
	@echo "  examples      - Generate example programs"
	@echo "  test-examples - Test all example programs"
	@echo "  runtime-info  - Show runtime library symbols"
	@echo "  runtime-stats - Show runtime library statistics"
	@echo "  debug         - Build with debug flags"
	@echo "  release       - Build with optimizations"
	@echo "  clean         - Clean all build artifacts"
	@echo "  help          - Show this help message"

# Phony targets
.PHONY: all clean test test-arrays test-strings test-builtins test-all test-verbose test-file test-clean
.PHONY: runtime compiler runtime-info runtime-stats lint-runtime format-runtime test-memory
.PHONY: examples test-examples debug release help