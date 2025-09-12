#include "runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>    // for memcpy(), memset(), memmove()
#include <stdint.h>    // for int64_t

// ===========================================
// ARRAY CREATION AND MANAGEMENT
// ===========================================

Array* array_create(int64_t element_size, int64_t initial_capacity) {
    if (element_size <= 0) {
        runtime_error("Invalid element size for array creation");
        return NULL;
    }
    
    Array* arr = safe_malloc(sizeof(Array));
    
    arr->length = 0;
    arr->capacity = initial_capacity > 0 ? initial_capacity : 4;  // Default capacity
    arr->data = safe_malloc(arr->capacity * element_size);
    
    // Initialize memory to zero
    memset(arr->data, 0, arr->capacity * element_size);
    
    return arr;
}

void array_free(Array* arr) {
    if (arr) {
        free(arr->data);
        free(arr);
    }
}

// ===========================================
// ARRAY RESIZING (INTERNAL)
// ===========================================

static void array_resize(Array* arr, int64_t element_size, int64_t new_capacity) {
    if (!arr || new_capacity <= 0) return;
    
    arr->data = safe_realloc(arr->data, new_capacity * element_size);
    
    // Initialize new memory if growing
    if (new_capacity > arr->capacity) {
        char* data_ptr = (char*)arr->data;
        memset(data_ptr + (arr->capacity * element_size), 0, 
               (new_capacity - arr->capacity) * element_size);
    }
    
    arr->capacity = new_capacity;
}

// ===========================================
// ARRAY ACCESS OPERATIONS
// ===========================================

void* array_get(Array* arr, int64_t index, int64_t element_size) {
    if (!arr) {
        runtime_error("Null array in array_get");
        return NULL;
    }
    
    if (index < 0 || index >= arr->length) {
        bounds_check_error(index, arr->length);
        exit(1);
    }
    
    char* data_ptr = (char*)arr->data;
    return data_ptr + (index * element_size);
}

void array_set(Array* arr, int64_t index, void* element, int64_t element_size) {
    if (!arr || !element) {
        runtime_error("Null pointer in array_set");
        return;
    }
    
    if (index < 0 || index >= arr->length) {
        bounds_check_error(index, arr->length);
        exit(1);
    }
    
    char* data_ptr = (char*)arr->data;
    memcpy(data_ptr + (index * element_size), element, element_size);
}

// ===========================================
// ARRAY MODIFICATION OPERATIONS
// ===========================================

void array_push(Array* arr, void* element, int64_t element_size) {
    if (!arr || !element) {
        runtime_error("Null pointer in array_push");
        return;
    }
    
    // Resize if necessary (double capacity when full)
    if (arr->length >= arr->capacity) {
        int64_t new_capacity = arr->capacity * 2;
        array_resize(arr, element_size, new_capacity);
    }
    
    // Copy element to end of array
    char* data_ptr = (char*)arr->data;
    memcpy(data_ptr + (arr->length * element_size), element, element_size);
    arr->length++;
}

void* array_pop(Array* arr, int64_t element_size) {
    if (!arr) {
        runtime_error("Null array in array_pop");
        return NULL;
    }
    
    if (arr->length == 0) {
        runtime_error("Cannot pop from empty array");
        return NULL;
    }
    
    arr->length--;
    char* data_ptr = (char*)arr->data;
    void* element = data_ptr + (arr->length * element_size);
    
    // Optionally shrink array if it becomes too sparse
    if (arr->length > 0 && arr->length < arr->capacity / 4 && arr->capacity > 8) {
        array_resize(arr, element_size, arr->capacity / 2);
    }
    
    return element;
}

// ===========================================
// ARRAY UTILITY OPERATIONS
// ===========================================

int64_t array_length(Array* arr) {
    return arr ? arr->length : 0;
}

int64_t array_capacity(Array* arr) {
    return arr ? arr->capacity : 0;
}

void array_clear(Array* arr, int64_t element_size) {
    if (arr) {
        arr->length = 0;
        // Optionally zero out memory
        if (arr->data) {
            memset(arr->data, 0, arr->capacity * element_size);
        }
    }
}

// ===========================================
// SPECIALIZED ARRAY OPERATIONS
// ===========================================

// Insert element at specific index
void array_insert(Array* arr, int64_t index, void* element, int64_t element_size) {
    if (!arr || !element) {
        runtime_error("Null pointer in array_insert");
        return;
    }
    
    if (index < 0 || index > arr->length) {
        bounds_check_error(index, arr->length + 1);  // +1 because insert can be at end
        exit(1);
    }
    
    // Resize if necessary
    if (arr->length >= arr->capacity) {
        array_resize(arr, element_size, arr->capacity * 2);
    }
    
    char* data_ptr = (char*)arr->data;
    
    // Shift elements to the right
    if (index < arr->length) {
        memmove(data_ptr + ((index + 1) * element_size),
                data_ptr + (index * element_size),
                (arr->length - index) * element_size);
    }
    
    // Insert new element
    memcpy(data_ptr + (index * element_size), element, element_size);
    arr->length++;
}

// Remove element at specific index
void array_remove(Array* arr, int64_t index, int64_t element_size) {
    if (!arr) {
        runtime_error("Null array in array_remove");
        return;
    }
    
    if (index < 0 || index >= arr->length) {
        bounds_check_error(index, arr->length);
        exit(1);
    }
    
    char* data_ptr = (char*)arr->data;
    
    // Shift elements to the left
    if (index < arr->length - 1) {
        memmove(data_ptr + (index * element_size),
                data_ptr + ((index + 1) * element_size),
                (arr->length - index - 1) * element_size);
    }
    
    arr->length--;
    
    // Optionally shrink array
    if (arr->length > 0 && arr->length < arr->capacity / 4 && arr->capacity > 8) {
        array_resize(arr, element_size, arr->capacity / 2);
    }
}