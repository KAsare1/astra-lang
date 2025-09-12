#include "runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>    // for strlen(), strcmp()
#include <errno.h>     // for errno
#include <stdint.h>    // for int64_t

// ===========================================
// I/O BUILT-IN FUNCTIONS
// ===========================================

String* input_string(void) {
    char buffer[1024];
    
    if (fgets(buffer, sizeof(buffer), stdin)) {
        // Remove newline if present
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
        }
        return string_create(buffer);
    }
    
    // Return empty string on error or EOF
    return string_create("");
}

void println_string(String* str) {
    if (str && str->data) {
        printf("%.*s\n", (int)str->length, str->data);
    } else {
        printf("\n");
    }
}

void println_int(int64_t value) {
    printf("%lld\n", value);
}

void println_double(double value) {
    printf("%g\n", value);
}

// ===========================================
// TYPE CONVERSION BUILT-INS
// ===========================================

String* int_to_string(int64_t value) {
    char buffer[32];
    int len = snprintf(buffer, sizeof(buffer), "%lld", value);
    
    if (len < 0 || len >= (int)sizeof(buffer)) {
        return string_create("0");  // Fallback
    }
    
    return string_create(buffer);
}

String* double_to_string(double value) {
    char buffer[32];
    int len = snprintf(buffer, sizeof(buffer), "%g", value);
    
    if (len < 0 || len >= (int)sizeof(buffer)) {
        return string_create("0.0");  // Fallback
    }
    
    return string_create(buffer);
}

String* bool_to_string(bool value) {
    return string_create(value ? "true" : "false");
}

// Generic to_string function (type-erased)
String* to_string_any(void* value) {
    // This is a simplified version - in practice would need type information
    // For now, treat as pointer address
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%p", value);
    return string_create(buffer);
}

int64_t string_to_int(String* str) {
    if (!str || !str->data || str->length == 0) {
        runtime_error("Cannot convert empty string to integer");
        return 0;
    }
    
    // Ensure null termination for strtoll
    char* temp = safe_malloc(str->length + 1);
    memcpy(temp, str->data, str->length);
    temp[str->length] = '\0';
    
    char* endptr;
    errno = 0;
    int64_t result = strtoll(temp, &endptr, 10);
    
    // Check for conversion errors
    if (errno != 0 || endptr == temp || *endptr != '\0') {
        fprintf(stderr, "Error: Invalid integer format: '%s'\n", temp);
        free(temp);
        return 0;
    }
    
    free(temp);
    return result;
}

double string_to_double(String* str) {
    if (!str || !str->data || str->length == 0) {
        runtime_error("Cannot convert empty string to double");
        return 0.0;
    }
    
    // Ensure null termination for strtod
    char* temp = safe_malloc(str->length + 1);
    memcpy(temp, str->data, str->length);
    temp[str->length] = '\0';
    
    char* endptr;
    errno = 0;
    double result = strtod(temp, &endptr);
    
    // Check for conversion errors
    if (errno != 0 || endptr == temp || *endptr != '\0') {
        fprintf(stderr, "Error: Invalid double format: '%s'\n", temp);
        free(temp);
        return 0.0;
    }
    
    free(temp);
    return result;
}

// ===========================================
// GENERIC LENGTH FUNCTION
// ===========================================

int64_t builtin_len(void* obj, const char* type_hint) {
    if (!obj) return 0;
    
    // Determine object type and return appropriate length
    if (type_hint) {
        if (strstr(type_hint, "array") || type_hint[0] == '[') {
            Array* arr = (Array*)obj;
            return arr->length;
        } else if (strstr(type_hint, "string")) {
            String* str = (String*)obj;
            return str->length;
        }
    }
    
    // Fallback: try to determine type by inspection
    // This is a heuristic and not foolproof
    Array* as_array = (Array*)obj;
    String* as_string = (String*)obj;
    
    // Simple heuristic: if capacity makes sense for a string, treat as string
    if (as_string->capacity > 0 && as_string->capacity < 1000000 && 
        as_string->length >= 0 && as_string->length < as_string->capacity) {
        return as_string->length;
    }
    
    // Otherwise treat as array
    if (as_array->capacity > 0 && as_array->length >= 0 && as_array->length <= as_array->capacity) {
        return as_array->length;
    }
    
    return 0;  // Unknown type
}

// Type-specific len functions (called by IR codegen)
int64_t len_array(Array* arr) {
    return arr ? arr->length : 0;
}

int64_t len_string(String* str) {
    return str ? str->length : 0;
}

// ===========================================
// ARRAY BUILT-IN FUNCTIONS
// ===========================================

// Push function for different element types
void push_int_array(Array* arr, int64_t element) {
    if (!arr) {
        runtime_error("Null array in push_int_array");
        return;
    }
    array_push(arr, &element, sizeof(int64_t));
}

void push_double_array(Array* arr, double element) {
    if (!arr) {
        runtime_error("Null array in push_double_array");
        return;
    }
    array_push(arr, &element, sizeof(double));
}

void push_string_array(Array* arr, String* element) {
    if (!arr) {
        runtime_error("Null array in push_string_array");
        return;
    }
    array_push(arr, &element, sizeof(String*));
}

// Generic push function (type-erased)
void push_generic(Array* arr, void* element, int64_t element_size) {
    if (!arr || !element) {
        runtime_error("Null pointer in push_generic");
        return;
    }
    array_push(arr, element, element_size);
}

// Pop function for different element types
int64_t pop_int_array(Array* arr) {
    if (!arr) {
        runtime_error("Null array in pop_int_array");
        return 0;
    }
    
    if (arr->length == 0) {
        runtime_error("Cannot pop from empty array");
        return 0;
    }
    
    void* element_ptr = array_pop(arr, sizeof(int64_t));
    return *(int64_t*)element_ptr;
}

double pop_double_array(Array* arr) {
    if (!arr) {
        runtime_error("Null array in pop_double_array");
        return 0.0;
    }
    
    if (arr->length == 0) {
        runtime_error("Cannot pop from empty array");
        return 0.0;
    }
    
    void* element_ptr = array_pop(arr, sizeof(double));
    return *(double*)element_ptr;
}

String* pop_string_array(Array* arr) {
    if (!arr) {
        runtime_error("Null array in pop_string_array");
        return NULL;
    }
    
    if (arr->length == 0) {
        runtime_error("Cannot pop from empty array");
        return NULL;
    }
    
    void* element_ptr = array_pop(arr, sizeof(String*));
    return *(String**)element_ptr;
}

// ===========================================
// STRING BUILT-IN FUNCTIONS
// ===========================================

String* builtin_concat(String* left, String* right) {
    return string_concat(left, right);
}

String* builtin_substr(String* str, int64_t start, int64_t length) {
    return string_substr(str, start, length);
}

// ===========================================
// UTILITY FUNCTIONS FOR IR CODEGEN
// ===========================================

// Create arrays of specific types (called by IR codegen)
Array* create_int_array(int64_t capacity) {
    return array_create(sizeof(int64_t), capacity);
}

Array* create_double_array(int64_t capacity) {
    return array_create(sizeof(double), capacity);
}

Array* create_string_array(int64_t capacity) {
    return array_create(sizeof(String*), capacity);
}

// Get/set functions for different array types
int64_t get_int_from_array(Array* arr, int64_t index) {
    void* ptr = array_get(arr, index, sizeof(int64_t));
    return ptr ? *(int64_t*)ptr : 0;
}

double get_double_from_array(Array* arr, int64_t index) {
    void* ptr = array_get(arr, index, sizeof(double));
    return ptr ? *(double*)ptr : 0.0;
}

String* get_string_from_array(Array* arr, int64_t index) {
    void* ptr = array_get(arr, index, sizeof(String*));
    return ptr ? *(String**)ptr : NULL;
}

void set_int_in_array(Array* arr, int64_t index, int64_t value) {
    array_set(arr, index, &value, sizeof(int64_t));
}

void set_double_in_array(Array* arr, int64_t index, double value) {
    array_set(arr, index, &value, sizeof(double));
}

void set_string_in_array(Array* arr, int64_t index, String* value) {
    array_set(arr, index, &value, sizeof(String*));
}

// ===========================================
// COMPARISON FUNCTIONS FOR BUILT-INS
// ===========================================

bool builtin_equals_int(int64_t left, int64_t right) {
    return left == right;
}

bool builtin_equals_double(double left, double right) {
    // Use epsilon comparison for floating point
    const double epsilon = 1e-10;
    return (left - right) < epsilon && (right - left) < epsilon;
}

bool builtin_equals_string(String* left, String* right) {
    return string_equals(left, right);
}

int builtin_compare_int(int64_t left, int64_t right) {
    if (left < right) return -1;
    if (left > right) return 1;
    return 0;
}

int builtin_compare_double(double left, double right) {
    if (left < right) return -1;
    if (left > right) return 1;
    return 0;
}

int builtin_compare_string(String* left, String* right) {
    return string_compare(left, right);
}

// ===========================================
// ADVANCED ARRAY OPERATIONS
// ===========================================

// Array slice function (creates a new array with a subset of elements)
Array* slice_int_array(Array* arr, int64_t start, int64_t end) {
    if (!arr || start < 0 || end < start || start >= arr->length) {
        return create_int_array(0);
    }
    
    if (end > arr->length) end = arr->length;
    
    int64_t slice_length = end - start;
    Array* slice = create_int_array(slice_length);
    
    int64_t* src_data = (int64_t*)arr->data;
    int64_t* dst_data = (int64_t*)slice->data;
    
    for (int64_t i = 0; i < slice_length; i++) {
        dst_data[i] = src_data[start + i];
    }
    
    slice->length = slice_length;
    return slice;
}

Array* slice_string_array(Array* arr, int64_t start, int64_t end) {
    if (!arr || start < 0 || end < start || start >= arr->length) {
        return create_string_array(0);
    }
    
    if (end > arr->length) end = arr->length;
    
    int64_t slice_length = end - start;
    Array* slice = create_string_array(slice_length);
    
    String** src_data = (String**)arr->data;
    String** dst_data = (String**)slice->data;
    
    for (int64_t i = 0; i < slice_length; i++) {
        dst_data[i] = src_data[start + i];  // Shallow copy of string pointers
    }
    
    slice->length = slice_length;
    return slice;
}

// Array copy function (deep copy)
Array* copy_int_array(Array* arr) {
    if (!arr) return NULL;
    
    Array* copy = create_int_array(arr->capacity);
    copy->length = arr->length;
    
    if (arr->length > 0) {
        memcpy(copy->data, arr->data, arr->length * sizeof(int64_t));
    }
    
    return copy;
}

Array* copy_string_array(Array* arr) {
    if (!arr) return NULL;
    
    Array* copy = create_string_array(arr->capacity);
    copy->length = arr->length;
    
    String** src_data = (String**)arr->data;
    String** dst_data = (String**)copy->data;
    
    // Deep copy strings
    for (int64_t i = 0; i < arr->length; i++) {
        if (src_data[i]) {
            dst_data[i] = string_create(string_to_cstr(src_data[i]));
        } else {
            dst_data[i] = NULL;
        }
    }
    
    return copy;
}

// ===========================================
// FUNCTIONAL PROGRAMMING HELPERS
// ===========================================

// Find element in array (returns index or -1)
int64_t find_int_in_array(Array* arr, int64_t value) {
    if (!arr) return -1;
    
    int64_t* data = (int64_t*)arr->data;
    for (int64_t i = 0; i < arr->length; i++) {
        if (data[i] == value) {
            return i;
        }
    }
    return -1;
}

int64_t find_string_in_array(Array* arr, String* value) {
    if (!arr || !value) return -1;
    
    String** data = (String**)arr->data;
    for (int64_t i = 0; i < arr->length; i++) {
        if (data[i] && string_equals(data[i], value)) {
            return i;
        }
    }
    return -1;
}

// Check if array contains element
bool contains_int_in_array(Array* arr, int64_t value) {
    return find_int_in_array(arr, value) >= 0;
}

bool contains_string_in_array(Array* arr, String* value) {
    return find_string_in_array(arr, value) >= 0;
}

// ===========================================
// CLEANUP AND DESTRUCTION FUNCTIONS
// ===========================================

// Free string array (including the strings themselves)
void free_string_array(Array* arr) {
    if (!arr) return;
    
    String** data = (String**)arr->data;
    for (int64_t i = 0; i < arr->length; i++) {
        if (data[i]) {
            string_free(data[i]);
        }
    }
    
    array_free(arr);
}

// Generic cleanup function
void cleanup_array(Array* arr, const char* element_type) {
    if (!arr) return;
    
    if (strcmp(element_type, "string") == 0 || strstr(element_type, "String")) {
        free_string_array(arr);
    } else {
        array_free(arr);
    }
}

// ===========================================
// RUNTIME PERFORMANCE MONITORING (OPTIONAL)
// ===========================================

#ifdef DEBUG
static int64_t allocation_count = 0;
static int64_t deallocation_count = 0;

void increment_allocation_count(void) {
    allocation_count++;
}

void increment_deallocation_count(void) {
    deallocation_count++;
}

void print_memory_stats(void) {
    printf("Memory Stats: Allocations=%lld, Deallocations=%lld, Net=%lld\n",
           allocation_count, deallocation_count, allocation_count - deallocation_count);
}
#else
void increment_allocation_count(void) {}
void increment_deallocation_count(void) {}
void print_memory_stats(void) {}
#endif

// ===========================================
// ENTRY POINT FOR RUNTIME INITIALIZATION
// ===========================================

void runtime_initialize(void) {
    // Initialize any global runtime state if needed
    // For now, this is a no-op but could be extended
    #ifdef DEBUG
    printf("Astra runtime initialized\n");
    #endif
}

void runtime_cleanup(void) {
    // Clean up any global runtime state if needed
    #ifdef DEBUG
    print_memory_stats();
    printf("Astra runtime cleaned up\n");
    #endif
}