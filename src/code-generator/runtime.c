#include "runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// ===========================================
// EXISTING BASIC I/O FUNCTIONS
// ===========================================

void print_i64(int64_t x) { 
    printf("%lld", x); 
}

void print_double(double x) { 
    printf("%g", x); 
}

void print_str(const char* s) { 
    printf("%s", s); 
}

// ===========================================
// ENHANCED I/O FUNCTIONS
// ===========================================

void println_any(void* value) {
    // Generic println - would need type info in real implementation
    // For now, just print newline (actual implementation depends on type system)
    printf("\n");
}

void print_string_struct(String* str) {
    if (str && str->data && str->length > 0) {
        printf("%.*s", (int)str->length, str->data);
    }
}

void println_string_struct(String* str) {
    print_string_struct(str);
    printf("\n");
}


void* safe_malloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr && size > 0) {
        fprintf(stderr, "Error: Memory allocation failed (%zu bytes)\n", size);
        exit(1);
    }
    return ptr;
}

void* safe_realloc(void* ptr, size_t size) {
    void* new_ptr = realloc(ptr, size);
    if (!new_ptr && size > 0) {
        fprintf(stderr, "Error: Memory reallocation failed (%zu bytes)\n", size);
        exit(1);
    }
    return new_ptr;
}


void bounds_check_error(int64_t index, int64_t length) {
    fprintf(stderr, "Runtime Error: Array index out of bounds\n");
    fprintf(stderr, "  Index: %lld, Array length: %lld\n", index, length);
    fprintf(stderr, "  Valid range: 0 to %lld\n", length - 1);
}

void runtime_error(const char* message) {
    fprintf(stderr, "Runtime Error: %s\n", message);
    exit(1);
}


void debug_print_array(Array* arr, int64_t element_size, const char* type) {
    if (!arr) {
        printf("Array: NULL\n");
        return;
    }
    
    printf("Array<%s>: length=%lld, capacity=%lld, data=%p\n", 
           type, arr->length, arr->capacity, arr->data);
    
    if (strcmp(type, "int") == 0 && arr->data) {
        int64_t* data = (int64_t*)arr->data;
        printf("Elements: [");
        for (int64_t i = 0; i < arr->length; i++) {
            if (i > 0) printf(", ");
            printf("%lld", data[i]);
        }
        printf("]\n");
    }
}

void debug_print_string(String* str) {
    if (!str) {
        printf("String: NULL\n");
        return;
    }
    
    printf("String: length=%lld, capacity=%lld, data=\"%.*s\"\n", 
           str->length, str->capacity, (int)str->length, str->data ? str->data : "");
}