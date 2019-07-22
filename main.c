#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>
#include "light_array.h"
#include "utils.h"
#include "cmd_line.h"

typedef struct {
    catstring filename;
    int       file_size_bytes;

    int is_directory;
    int line_count;
    int comment_count;
    int blank_count;
} HOLC_File;

HOLC_File
holc_count_file_lines(catstring filename) {
    HOLC_File stats = {0};
    int error = 0, file_size = 0;

    char* data = os_read_entire_file(filename.data, &file_size, &error);

    stats.filename = filename;
    stats.file_size_bytes = file_size;

    if(file_size == 0) 
        return stats;

    int line_count = 1;
    int blank_count = 0;
    int comment_count = 0;

    int char_count = 0;
    int has_comment = 0;
    for(int i = 0; i < file_size; ++i) 
    {
        char c = data[i];
        switch(c) 
        {
            case 0: break;
            // Might be a comment
            case '/':{
                i++;
                char di = data[i];
                if(di) {
                    if(di == '/') {
                        // single line comment
                        has_comment = 1;
                        while(data[i] && data[i] != '\n') {
                            i++;
                        }
                        i--;
                    } else if(di == '*') {
                        // multi line comment
                        has_comment = 1;
                        i++;
                        while(data[i]) {
                            if(data[i] == '*') {
                                i++;
                                if(data[i] && data[i] == '/') {
                                    break;
                                }
                            } else if(data[i] == '\n') {
                                line_count++;
                                comment_count++;
                            }
                            i++;
                        }
                    }
                }
                
            } break;
            
            // skip whitespace
            case ' ': case '\r': case '\t': case '\f': case '\v':
                break;

            case '\n': {
                if(char_count == 0) {
                    if(has_comment) {
                        has_comment = 0;
                        comment_count++;
                    } else {
                        blank_count++;
                    }
                }
                char_count = 0;
                line_count++;
            } break;

            default: 
                char_count++; 
                break;
        }
    }
    if(char_count == 0) {
        if(has_comment) {
            comment_count++;
        } else {
            blank_count++;
        }
    }

    free(data);

    stats.blank_count = blank_count;
    stats.comment_count = comment_count;
    stats.line_count = line_count;

    return stats;
}

int 
holc_is_excluded_file(const char* filename, const char** excluded_files, int excluded_files_count) {
    for(int i = 0; i < excluded_files_count; ++i) {
        if(strcmp(filename, excluded_files[i]) == 0) return 1;
    }
    return 0;
}

typedef struct {
    int dir_count;
    int file_count;
    int max_filename_length;

    catstring* file_names;
    HOLC_File* files;

    int line_count;
    int comment_count;
    int blank_count;
} Global_Files;

static Global_Files global_files;

void
holc_gather_files(
    const char* filename, 
    const char** ext, int ext_count, 
    const char** excluded_dirs, int excluded_dirs_count,
    const char** excluded_files, int excluded_files_count,
    catstring* current_dir,
    int flags)
{
    catsprint(current_dir, (char*)filename);
    catsprint(current_dir, "/");
    int current_dir_length = current_dir->length;

    DIR *d;
    struct dirent *dir;
    d = opendir(current_dir->data);

    //printf("%s\n", current_dir->data);

    if (d) {
        while ((dir = readdir(d)) != NULL) 
        {
            switch(dir->d_type) {
                case DT_LNK: // symbolic link
                case DT_REG: // regular file
                {
                    if(!(flags & HOLC_FLAG_DO_HIDDEN_DIRS) && *dir->d_name == '.') {
                        break;
                    }
                    if(file_extension_match(dir->d_name, ext, ext_count) &&
                        !holc_is_excluded_file(dir->d_name, excluded_files, excluded_files_count)) 
                    {
                        // do nothing for now
                        global_files.file_count++;
                        catstring filen = catstring_copy(current_dir);
                        catsprint(&filen, dir->d_name);
                        array_push(global_files.file_names, filen);
                    }
                } break;
                case DT_DIR: // directory
                {
                    if(strcmp(".", dir->d_name) == 0) {
                        break;
                    }
                    if(strcmp("..", dir->d_name) == 0) {
                        break;
                    }
                    if(!(flags & HOLC_FLAG_DO_HIDDEN_DIRS) && *dir->d_name == '.') {
                        break;
                    }
                    if(flags & HOLC_FLAG_RECURSIVE) {
                        if(!holc_is_excluded_file(dir->d_name, excluded_dirs, excluded_dirs_count))
                        {
                            global_files.dir_count++;
                            holc_gather_files(dir->d_name, ext, ext_count,
                                excluded_dirs, excluded_dirs_count, 
                                excluded_files, excluded_files_count,
                                current_dir,
                                flags);
                            current_dir->length = current_dir_length;
                        }
                    }
                } break;
                default: break;
            }
        }
    }

    closedir(d);
}

static void
print_extra_chars(char c, int n) {
    for(int i = 0; i < n; ++i) {
        printf("%c", c);
    }
}

void
holc_print_node_stat(HOLC_File file, int column_length) {
    printf("%s", file.filename.data);
    print_extra_chars(' ', global_files.max_filename_length - file.filename.length);
    printf("   "); // separator

    int l = printf("%d", file.line_count);
    print_extra_chars(' ', column_length - l);
    global_files.line_count += file.line_count;
    
    l = printf("%d", file.line_count - (file.comment_count + file.blank_count));
    print_extra_chars(' ', column_length - l);

    l = printf("%d", file.comment_count);
    print_extra_chars(' ', column_length - l);
    global_files.comment_count += file.comment_count;

    l = printf("%d", file.blank_count);
    print_extra_chars(' ', column_length - l);
    global_files.blank_count += file.blank_count;

    printf("\n");
}

void
holc_print_array_stats(HOLC_File* results, int block_length) {
    if(!results || array_length(results) == 0) return;
    for(int i = 0; i < array_length(results); ++i) {
        HOLC_File n = results[i];
        holc_print_node_stat(n, block_length);
    }
}

static int compare_results_line(const void* left, const void* right)  {
    return ((HOLC_File*)right)->line_count - ((HOLC_File*)left)->line_count;
}
static int compare_results_comment(const void* left, const void* right)  {
    return ((HOLC_File*)right)->comment_count - ((HOLC_File*)left)->comment_count;
}
static int compare_results_blank(const void* left, const void* right)  {
    return ((HOLC_File*)right)->blank_count - ((HOLC_File*)left)->blank_count;
}
static int compare_results_code(const void* left, const void* right)  {
    int lc = ((HOLC_File*)left)->line_count - 
        (((HOLC_File*)left)->comment_count + ((HOLC_File*)left)->blank_count);
    int rc = ((HOLC_File*)right)->line_count - 
        (((HOLC_File*)right)->comment_count + ((HOLC_File*)right)->blank_count);
    return rc - lc;
}

void 
holc_results_sort(HOLC_File* results, int flags) {
    if(flags & HOLC_FLAG_SORT_COMMENT) {
        qsort(results, array_length(results), sizeof(HOLC_File), compare_results_comment);
    } else if(flags & HOLC_FLAG_SORT_BLANK) {
        qsort(results, array_length(results), sizeof(HOLC_File), compare_results_blank);
    } else if(flags & HOLC_FLAG_SORT_CODE) {
        qsort(results, array_length(results), sizeof(HOLC_File), compare_results_code);
    } else {
        qsort(results, array_length(results), sizeof(HOLC_File), compare_results_line);
    }
}

void
holc_gather_stats() {
    for(int i = 0; i < array_length(global_files.file_names); ++i) {
        catstring filename = global_files.file_names[i];
        if(filename.length > global_files.max_filename_length) {
            global_files.max_filename_length = filename.length;
        }
        HOLC_File f = holc_count_file_lines(filename);
        array_push(global_files.files, f);
    }
}

int main(int argc, char** argv) {
    if(argc < 2) {
        return print_usage(argv[0]);
    }

    Configuration config = parse_command_line(argc, argv);

    if(config.flags & HOLC_FLAG_HELP) {
        return print_help(argv[0]);
    }
    if(config.error) {
        return print_usage(argv[0]);
    }

    global_files.files = array_new(HOLC_File);
    global_files.file_names = array_new(catstring);
    catstring current_dir = {0};
    holc_gather_files(
        config.files_and_dirs[0], 
        (const char**)config.extensions, config.extension_count,
        (const char**)config.excluded_dirs, config.excluded_dir_count, 
        (const char**)config.excluded_files, config.excluded_files_count,
        &current_dir,
        config.flags);

    holc_gather_stats();

    int block_length = 12;
    int separator_length = 3;

    holc_results_sort(global_files.files, config.flags);

    // Header
    printf("filename");
    print_extra_chars(' ', global_files.max_filename_length + separator_length - strlen("filename"));
    printf("lines       code        comments    blank\n");
    print_extra_chars('-', global_files.max_filename_length + separator_length + block_length * 4);
    printf("\n");

    // Results
    holc_print_array_stats(global_files.files, block_length);

    // Footer
    print_extra_chars('-', global_files.max_filename_length + separator_length + block_length * 4);
    printf("\nfilename");
    print_extra_chars(' ', global_files.max_filename_length + separator_length - strlen("filename"));
    printf("lines       code        comments    blank\n");

    print_extra_chars('-', global_files.max_filename_length + separator_length + block_length * 4);
    printf("\n");
    print_extra_chars(' ', global_files.max_filename_length + separator_length);

    int l = printf("%d", global_files.line_count);
    print_extra_chars(' ', block_length - l);

    l = printf("%d", global_files.line_count - (global_files.comment_count + global_files.blank_count));
    print_extra_chars(' ', block_length - l);

    l = printf("%d", global_files.comment_count);
    print_extra_chars(' ', block_length - l);

    l = printf("%d", global_files.blank_count);
    print_extra_chars(' ', block_length - l);
    printf("\n");

    return 0;
}