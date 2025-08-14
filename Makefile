CC=gcc
CXX=g++
CFLAGS=-Wall -std=c11
CXXFLAGS=-Wall -std=c++17 -I$(LLVM_DIR)/include
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

.PHONY: all clean
