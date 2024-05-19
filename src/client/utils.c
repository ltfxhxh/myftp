// utils.c
#include "utils.h"
#include "common.h"

void check_cwd(char *cwd, size_t size) {
    if ((getcwd(cwd, size)) == NULL) {
        perror("getcwd error");
    }
}

