// utils.c
#include "utils.h"
#include "common.h"

void check_cwd(char *cwd, size_t size) {
    if ((getcwd(cwd, size)) == NULL) {
        perror("getcwd error");
    }
}

off_t get_file_size(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == 0) {
        return st.st_size;
    }

    return -1;
}

