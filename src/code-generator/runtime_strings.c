#include "runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>    // for strlen(), memcpy(), memcmp()
#include <ctype.h>     // for toupper(), tolower()
#include <stdint.h>    // for int64_t

// ===========================================
// STRING CREATION AND MANAGEMENT
// ===========================================

String* string_create(const char* cstr) {
    String* str = safe_malloc(sizeof(String));
    
    int64_t len = cstr ? strlen(cstr) : 0;
    str->length = len;
    str->capacity = len + 1;  // +1 for null terminator
    
    str->data = safe_malloc(str->capacity);
    
    if (cstr && len > 0) {
        memcpy(str->data, cstr, len);
    }
    str->data[len] = '\0';  // Always null terminate
    
    return str;
}

String* string_create_with_capacity(int64_t capacity) {
    String* str = safe_malloc(sizeof(String));
    
    str->length = 0;
    str->capacity = capacity > 0 ? capacity : 1;
    str->data = safe_malloc(str->capacity);
    str->data[0] = '\0';
    
    return str;
}

void string_free(String* str) {
    if (str) {
        free(str->data);
        free(str);
    }
}

// ===========================================
// STRING RESIZING (INTERNAL)
// ===========================================

static void string_resize(String* str, int64_t new_capacity) {
    if (!str || new_capacity <= str->length) return;
    
    str->data = safe_realloc(str->data, new_capacity);
    str->capacity = new_capacity;
}

// ===========================================
// STRING CONCATENATION
// ===========================================

String* string_concat(String* left, String* right) {
    if (!left || !right) {
        runtime_error("Null string in concatenation");
        return NULL;
    }
    
    String* result = safe_malloc(sizeof(String));
    
    result->length = left->length + right->length;
    result->capacity = result->length + 1;  // +1 for null terminator
    result->data = safe_malloc(result->capacity);
    
    // Copy left string
    if (left->length > 0) {
        memcpy(result->data, left->data, left->length);
    }
    
    // Copy right string
    if (right->length > 0) {
        memcpy(result->data + left->length, right->data, right->length);
    }
    
    // Null terminate
    result->data[result->length] = '\0';
    
    return result;
}

// In-place concatenation (modifies left string)
void string_append(String* left, String* right) {
    if (!left || !right) {
        runtime_error("Null string in append");
        return;
    }
    
    int64_t new_length = left->length + right->length;
    int64_t new_capacity = new_length + 1;
    
    // Resize if necessary
    if (new_capacity > left->capacity) {
        string_resize(left, new_capacity * 2);  // Grow with some extra space
    }
    
    // Append right string
    if (right->length > 0) {
        memcpy(left->data + left->length, right->data, right->length);
    }
    
    left->length = new_length;
    left->data[left->length] = '\0';
}

// ===========================================
// STRING SUBSTRING OPERATIONS
// ===========================================

String* string_substr(String* str, int64_t start, int64_t length) {
    if (!str) {
        runtime_error("Null string in substr");
        return NULL;
    }
    
    // Validate and clamp parameters
    if (start < 0) start = 0;
    if (start >= str->length) return string_create("");
    
    if (length < 0) length = 0;
    if (start + length > str->length) {
        length = str->length - start;
    }
    
    String* result = safe_malloc(sizeof(String));
    result->length = length;
    result->capacity = length + 1;
    result->data = safe_malloc(result->capacity);
    
    if (length > 0) {
        memcpy(result->data, str->data + start, length);
    }
    result->data[length] = '\0';
    
    return result;
}

// ===========================================
// STRING CHARACTER ACCESS
// ===========================================

char string_get_char(String* str, int64_t index) {
    if (!str) {
        runtime_error("Null string in get_char");
        return '\0';
    }
    
    if (index < 0 || index >= str->length) {
        bounds_check_error(index, str->length);
        exit(1);
    }
    
    return str->data[index];
}

void string_set_char(String* str, int64_t index, char c) {
    if (!str) {
        runtime_error("Null string in set_char");
        return;
    }
    
    if (index < 0 || index >= str->length) {
        bounds_check_error(index, str->length);
        exit(1);
    }
    
    str->data[index] = c;
}

// Get character as single-character string (for language consistency)
String* string_get_char_as_string(String* str, int64_t index) {
    char c = string_get_char(str, index);
    char temp[2] = {c, '\0'};
    return string_create(temp);
}

// ===========================================
// STRING UTILITY OPERATIONS
// ===========================================

int64_t string_length(String* str) {
    return str ? str->length : 0;
}

bool string_equals(String* left, String* right) {
    if (!left || !right) return false;
    if (left->length != right->length) return false;
    
    return memcmp(left->data, right->data, left->length) == 0;
}

int string_compare(String* left, String* right) {
    if (!left || !right) return 0;
    
    int64_t min_len = left->length < right->length ? left->length : right->length;
    int cmp = memcmp(left->data, right->data, min_len);
    
    if (cmp == 0) {
        // If common prefix is equal, compare lengths
        if (left->length < right->length) return -1;
        if (left->length > right->length) return 1;
        return 0;
    }
    
    return cmp;
}

// ===========================================
// STRING SEARCH OPERATIONS
// ===========================================

int64_t string_find(String* haystack, String* needle) {
    if (!haystack || !needle) return -1;
    if (needle->length == 0) return 0;
    if (needle->length > haystack->length) return -1;
    
    for (int64_t i = 0; i <= haystack->length - needle->length; i++) {
        if (memcmp(haystack->data + i, needle->data, needle->length) == 0) {
            return i;
        }
    }
    
    return -1;
}

bool string_contains(String* haystack, String* needle) {
    return string_find(haystack, needle) >= 0;
}

// ===========================================
// STRING MODIFICATION OPERATIONS
// ===========================================

String* string_to_upper(String* str) {
    if (!str) return NULL;
    
    String* result = string_create("");
    result->length = str->length;
    result->capacity = str->capacity;
    result->data = safe_realloc(result->data, result->capacity);
    
    for (int64_t i = 0; i < str->length; i++) {
        result->data[i] = toupper(str->data[i]);
    }
    result->data[result->length] = '\0';
    
    return result;
}

String* string_to_lower(String* str) {
    if (!str) return NULL;
    
    String* result = string_create("");
    result->length = str->length;
    result->capacity = str->capacity;
    result->data = safe_realloc(result->data, result->capacity);
    
    for (int64_t i = 0; i < str->length; i++) {
        result->data[i] = tolower(str->data[i]);
    }
    result->data[result->length] = '\0';
    
    return result;
}

// ===========================================
// STRING CONVERSION TO C STRING
// ===========================================

const char* string_to_cstr(String* str) {
    if (!str || !str->data) return "";
    
    // Ensure null termination
    if (str->length < str->capacity) {
        str->data[str->length] = '\0';
    }
    
    return str->data;
}