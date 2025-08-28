CC=gcc
CXX=g++
CFLAGS=-Wall -std=c11
CXXFLAGS=-Wall -std=c++17 -Isrc $(shell $(LLVM_DIR)/bin/llvm-config --cxxflags | sed 's/-fno-exceptions//g')
SRC_DIR=src
BUILD_DIR=build
TARGET=astra

LLVM_DIR=/opt/homebrew/opt/llvm
LDFLAGS=-L$(LLVM_DIR)/lib -Wl,-rpath,$(LLVM_DIR)/lib
LDLIBS=$(shell $(LLVM_DIR)/bin/llvm-config --libs all) $(shell $(LLVM_DIR)/bin/llvm-config --system-libs)

C_SRCS=$(shell find $(SRC_DIR) -name '*.c')
CXX_SRCS=$(shell find $(SRC_DIR) -name '*.cpp')
C_OBJS=$(C_SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
CXX_OBJS=$(CXX_SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
OBJS=$(C_OBJS) $(CXX_OBJS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS)

all: $(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

test: $(TARGET)
	@echo "=== Running Astra Compiler with Shared Symbol Table ==="
	./$(TARGET) tests/astra.acc
	@echo ""
	@echo "=== Linking with Runtime ==="
	gcc -c src/code-generator/runtime.c -o build/runtime.o
	gcc build/output.o build/runtime.o -o test_program
	@echo ""
	@echo "=== Running Compiled Program ==="
	./test_program
	@echo ""
	@echo "=== Test Complete ==="

# Verbose test that shows all intermediate files
test-verbose: $(TARGET)
	@echo "=== Running Astra Compiler (Verbose Mode) ==="
	./$(TARGET) tests/astra.acc
	@echo ""
	@echo "=== Generated LLVM IR ==="
	@cat build/output.ll
	@echo ""
	@echo "=== Generated Assembly (first 50 lines) ==="
	@head -50 build/output.s
	@echo ""
	@echo "=== Linking and Running ==="
	gcc -c src/code-generator/runtime.c -o build/runtime.o
	gcc build/output.o build/runtime.o -o test_program
	./test_program

# Test with different source files
test-file: $(TARGET)
	@if [ -z "$(FILE)" ]; then \
		echo "Usage: make test-file FILE=path/to/your/file.astra"; \
		exit 1; \
	fi
	./$(TARGET) $(FILE)
	gcc -c src/code-generator/runtime.c -o build/runtime.o
	gcc build/output.o build/runtime.o -o test_program
	./test_program

# Clean up all generated files
test-clean:
	rm -f test_program build/runtime.o build/output.*

.PHONY: test test-verbose test-file test-clean