#include "cmd_line.h"
#include "light_array.h"

Configuration
parse_command_line(int argc, char** argv) {
    Configuration result = {0};
    result.files_and_dirs = array_new(char*);
    result.excluded_dirs = array_new(char*);
    result.excluded_files = array_new(char*);
    result.extensions = array_new(char*);

    int i = 1;
    // files or directories
    for(; i < argc; ++i) {
        char* first = argv[i];
        if(*first && *first == '-') {
            break;
        }
        array_push(result.files_and_dirs, first);
        result.files_and_dirs_count++;
    }

    if(result.files_and_dirs_count == 0) {
        result.error = 1;
    }

    // flags
    for(; i < argc; ++i) {
        char* first = argv[i];
        switch(*first) {
            case 0: break;
            case '-':{
                if(strcmp("-e", first) == 0 || strcmp("--extensions", first) == 0) {
                    // file extensions
                    i++;
                    for(; i < argc; ++i) {
                        char* ext = argv[i];
                        if(*ext == '-') { i--; break; }
                        array_push(result.extensions, ext);
                        result.extension_count++;
                    }
                } else if(strcmp("-f", first) == 0 || strcmp("--files-excluded", first) == 0) {
                    // excluded files
                    i++;
                    for(; i < argc; ++i) {
                        char* dir = argv[i];
                        if(*dir == '-') { i--; break; }
                        array_push(result.excluded_files, dir);
                        result.excluded_files_count++;
                    }
                } else if(strcmp("-d", first) == 0 || strcmp("--dirs-excluded", first) == 0) {
                    // excluded dirs
                    i++;
                    for(; i < argc; ++i) {
                        char* dir = argv[i];
                        if(*dir == '-') { i--; break; }
                        array_push(result.excluded_dirs, dir);
                        result.excluded_dir_count++;
                    }
                } else if(strcmp("-r", first) == 0 || strcmp("--recursive", first) == 0) {
                    // recursive
                    result.flags |= HOLC_FLAG_RECURSIVE;
                } else if(strcmp("-h", first) == 0 || strcmp("--help", first) == 0) {
                    // help
                    result.flags |= HOLC_FLAG_HELP;
                } else if(strcmp("-a", first) == 0 || strcmp("--all", first) == 0) {
                    result.flags |= HOLC_FLAG_DO_HIDDEN_DIRS;
                } else if(strcmp("-s", first) == 0 || strcmp("--sort-by", first) == 0) {
                    i++;
                    char* arg = argv[i];
                    if(arg && *arg != '-') {
                        if(strcmp(arg, "comment") == 0) {
                            result.flags |= HOLC_FLAG_SORT_COMMENT;
                        } else if(strcmp(arg, "code") == 0) {
                            result.flags |= HOLC_FLAG_SORT_CODE;
                        } else if(strcmp(arg, "blank") == 0) {
                            result.flags |= HOLC_FLAG_SORT_BLANK;
                        } else if(strcmp(arg, "line") == 0) {
                            result.flags |= HOLC_FLAG_SORT_LINE;
                        }
                    }
                } else {
                    fprintf(stderr, "ignoring unrecognized '%s' command\n", first);
                }
            } break;
            default: {
                fprintf(stderr, "ignoring '%s' command argument\n", first);
            } break;
        }
    }

    if(result.extension_count == 0) {
        array_push(result.extensions, "c");
        array_push(result.extensions, "cpp");
        array_push(result.extensions, "h");
        array_push(result.extensions, "hpp");
        result.extension_count = 4;
    }

    return result;
}

int 
print_usage(const char* filename) {
    fprintf(stderr, "usage: %s path [options...]\n", filename);
    fprintf(stderr, " run with --help or -h for help\n");
    return 1;
}

int
print_help(const char* filename) {
    print_usage(filename);
    
    printf("\n");
    printf("Options for ho line counter are:\n\n");
    printf("-a, --all                            Include hidden files and directories\n");
    printf("-r, --recursive                      Search directories recursively\n");
    printf("-s, --sort-by <metric>               Sort the results by an specific metric\n");
    printf("                                     available metrics are: line, comment,\n");
    printf("                                     code and blank.\n");
    printf("-e, --extensions <ext list...>       List of file extensions to filter by.\n");
    printf("                                     defaults are: c, cpp, h, hpp\n");
    printf("-d, --dirs-excluded <dir list...>    List of directories to be excluded by the search.\n");
    printf("-f, --files-excluded <file list...>  List of files to be excluded by the search.\n");

    return 0;
}