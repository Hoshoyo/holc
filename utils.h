#pragma once

void*       os_read_entire_file(const char* filename, int* out_size, int* error);
const char* file_extension(const char* filename);
int         file_extension_match(const char* filename, const char** ext, int ext_count);

typedef struct {
    int   length;
    int   capacity;
    char* data;
} catstring;

int       catsprint(catstring* buffer, char* str);
catstring catstring_copy(catstring* s);