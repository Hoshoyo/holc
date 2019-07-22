#pragma once

typedef struct {
    char** files_and_dirs;
    char** excluded_dirs;
    char** excluded_files;
    char** extensions;

    int    files_and_dirs_count;
    int    extension_count;
    int    excluded_dir_count;
    int    excluded_files_count;

    int    flags;
    int    error;
} Configuration;

enum {
    HOLC_FLAG_NONE          = 0,
    HOLC_FLAG_VERBOSE       = (1 << 1),
    HOLC_FLAG_RECURSIVE     = (1 << 2),
    HOLC_FLAG_SORT_LINE     = (1 << 3),
    HOLC_FLAG_SORT_COMMENT  = (1 << 4),
    HOLC_FLAG_SORT_BLANK    = (1 << 5),
    HOLC_FLAG_SORT_CODE     = (1 << 6),
    HOLC_FLAG_HELP          = (1 << 7),
    HOLC_FLAG_DO_HIDDEN_DIRS = (1 << 8),
};

Configuration parse_command_line(int argc, char** argv);

int print_usage(const char* filename);
int print_help(const char* filename);