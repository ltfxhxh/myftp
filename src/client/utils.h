// utils.h
#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>

void check_cwd(char *cwd, size_t size);

// 得到本地文件大小
off_t get_file_size(const char *filename);

#endif // UTILS_H

