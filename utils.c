#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

void*
os_read_entire_file(const char* filename, int* out_size, int* error) {
    FILE* file = fopen(filename, "rb");

    if(!file) {
        fprintf(stderr, "Could not find file %s\n", filename);
        if(error) *error = 1;
        return 0;
    }

    fseek(file, 0, SEEK_END);
    int file_size = (int)ftell(file);
    fseek(file, 0, SEEK_SET);

    if(file_size == 0) {
        fclose(file);
        if(out_size) *out_size = 0;
        return 0;
    }

    void* memory = calloc(1, file_size + 1);

    if(fread(memory, file_size, 1, file) != 1) {
        fclose(file);
        free(memory);
        fprintf(stderr, "Could not read entire file %s\n", filename);
        if(error) *error = 1;
        return 0;
    }
    fclose(file);

    if(out_size) *out_size = file_size;
    return memory;
}

const char*
file_extension(const char* filename) {
    int len = strlen(filename);
    int i = len - 1;
    for(; i >= 0; --i) {
        if(filename[i] == '.') {
            i++; // skip .
            break;
        }
    }
    return filename + i;
}

int 
file_extension_match(const char* filename, const char** ext, int ext_count) {
    for(int i = 0; i < ext_count; ++i) {
        const char* e = file_extension(filename);
        if(strcmp(ext[i], e) == 0) {
            return 1;
        }
    }
    return 0;
}

// --------------------------------
// ---------- Catstring -----------
// --------------------------------

static void
buffer_grow(catstring* s) {
    if(s->capacity < 32) {
        s->data = realloc(s->data, s->capacity + 32);
        s->capacity = s->capacity + 32;
    } else {
        s->data = realloc(s->data, s->capacity * 2);
        s->capacity = s->capacity * 2;
    }
}

int 
catsprint(catstring* buffer, char* str) {
    if(buffer->capacity == 0) {
        buffer->data = calloc(1, 32);
        buffer->capacity = 32;
    }

    int n = 0;
    for(char* at = str;; ++at, ++n) {
        // push to stream
        if(buffer->length >= buffer->capacity) {
            buffer_grow(buffer);
        }

        buffer->data[buffer->length] = *at;
        if(!(*at)) break;
        buffer->length++;
    }

    return n;
}

catstring 
catstring_copy(catstring* s) {
    catstring result = {0};

    result.data = calloc(1, s->capacity);
    result.capacity = s->capacity;
    result.length = s->length;
    memcpy(result.data, s->data, s->length);

    return result;
}