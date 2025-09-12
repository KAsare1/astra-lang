// ===========================================
// src/code-generator/runtime.h
// ===========================================

#ifndef ASTRA_RUNTIME_H
#define ASTRA_RUNTIME_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// ===========================================
// DATA STRUCTURE DEFINITIONS
// ===========================================

// String structure: { length, data, capacity }
typedef struct {
    int64_t length;
    char* data;
    int64_t capacity;
} String;

// Generic array structure: { length, capacity, data }
typedef struct {
    int64_t length;
    int64_t capacity;
    void* data;
} Array;

// ===========================================
// CORE RUNTIME FUNCTIONS (runtime.c)
// ===========================================

// Basic I/O functions (existing)
void print_i64(int64_t x);
void print_double(double x);
void print_str(const char* s);

// Enhanced I/O functions
void println_any(void* value);
void print_string_struct(String* str);
void println_string_struct(String* str);

// Memory management helpers
void* safe_malloc(size_t size);
void* safe_realloc(void* ptr, size_t size);

// Error handling and safety
void bounds_check_error(int64_t index, int64_t length);
void runtime_error(const char* message);

// Debug utilities
void debug_print_array(Array* arr, int64_t element_size, const char* type);
void debug_print_string(String* str);

// ===========================================
// ARRAY OPERATIONS (runtime_arrays.c)
// ===========================================

// Array creation and management
Array* array_create(int64_t element_size, int64_t initial_capacity);
void array_free(Array* arr);

// Array access operations
void* array_get(Array* arr, int64_t index, int64_t element_size);
void array_set(Array* arr, int64_t index, void* element, int64_t element_size);

// Array modification operations
void array_push(Array* arr, void* element, int64_t element_size);
void* array_pop(Array* arr, int64_t element_size);

// Array utility operations
int64_t array_length(Array* arr);
int64_t array_capacity(Array* arr);
void array_clear(Array* arr, int64_t element_size);

// Specialized array operations
void array_insert(Array* arr, int64_t index, void* element, int64_t element_size);
void array_remove(Array* arr, int64_t index, int64_t element_size);

// ===========================================
// STRING OPERATIONS (runtime_strings.c)
// ===========================================

// String creation and management
String* string_create(const char* cstr);
String* string_create_with_capacity(int64_t capacity);
void string_free(String* str);

// String concatenation
String* string_concat(String* left, String* right);
void string_append(String* left, String* right);

// String substring operations
String* string_substr(String* str, int64_t start, int64_t length);

// String character access
char string_get_char(String* str, int64_t index);
void string_set_char(String* str, int64_t index, char c);
String* string_get_char_as_string(String* str, int64_t index);

// String utility operations
int64_t string_length(String* str);
bool string_equals(String* left, String* right);
int string_compare(String* left, String* right);

// String search operations
int64_t string_find(String* haystack, String* needle);
bool string_contains(String* haystack, String* needle);

// String modification operations
String* string_to_upper(String* str);
String* string_to_lower(String* str);

// String conversion to C string
const char* string_to_cstr(String* str);

// ===========================================
// BUILT-IN FUNCTIONS (runtime_builtins.c)
// ===========================================

// I/O built-in functions
String* input_string(void);
void println_string(String* str);
void println_int(int64_t value);
void println_double(double value);

// Type conversion built-ins
String* int_to_string(int64_t value);
String* double_to_string(double value);
String* bool_to_string(bool value);
String* to_string_any(void* value);
int64_t string_to_int(String* str);
double string_to_double(String* str);

// Generic length function
int64_t builtin_len(void* obj, const char* type_hint);
int64_t len_array(Array* arr);
int64_t len_string(String* str);

// Array built-in functions
void push_int_array(Array* arr, int64_t element);
void push_double_array(Array* arr, double element);
void push_string_array(Array* arr, String* element);
void push_generic(Array* arr, void* element, int64_t element_size);

int64_t pop_int_array(Array* arr);
double pop_double_array(Array* arr);
String* pop_string_array(Array* arr);

// String built-in functions
String* builtin_concat(String* left, String* right);
String* builtin_substr(String* str, int64_t start, int64_t length);

// Utility functions for IR codegen
Array* create_int_array(int64_t capacity);
Array* create_double_array(int64_t capacity);
Array* create_string_array(int64_t capacity);

// Get/set functions for different array types
int64_t get_int_from_array(Array* arr, int64_t index);
double get_double_from_array(Array* arr, int64_t index);
String* get_string_from_array(Array* arr, int64_t index);
void set_int_in_array(Array* arr, int64_t index, int64_t value);
void set_double_in_array(Array* arr, int64_t index, double value);
void set_string_in_array(Array* arr, int64_t index, String* value);

// Comparison functions for built-ins
bool builtin_equals_int(int64_t left, int64_t right);
bool builtin_equals_double(double left, double right);
bool builtin_equals_string(String* left, String* right);
int builtin_compare_int(int64_t left, int64_t right);
int builtin_compare_double(double left, double right);
int builtin_compare_string(String* left, String* right);

// Advanced array operations
Array* slice_int_array(Array* arr, int64_t start, int64_t end);
Array* slice_string_array(Array* arr, int64_t start, int64_t end);
Array* copy_int_array(Array* arr);
Array* copy_string_array(Array* arr);

// Functional programming helpers
int64_t find_int_in_array(Array* arr, int64_t value);
int64_t find_string_in_array(Array* arr, String* value);
bool contains_int_in_array(Array* arr, int64_t value);
bool contains_string_in_array(Array* arr, String* value);

// Cleanup and destruction functions
void free_string_array(Array* arr);
void cleanup_array(Array* arr, const char* element_type);

// Runtime performance monitoring (optional)
void increment_allocation_count(void);
void increment_deallocation_count(void);
void print_memory_stats(void);

// Runtime initialization
void runtime_initialize(void);
void runtime_cleanup(void);

#endif // ASTRA_RUNTIME_H