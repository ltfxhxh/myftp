#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define ERROR_CHECK(client_fd, msg) do { \
    LOG_ERROR(msg); \
    perror(msg); \
    const char *error_msg = msg; \
    write(client_fd, error_msg, strlen(error_msg)); \
    write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE)); \
} while (0)

/*========== 处理命令行命令 ==========*/
// 目录跳转
void handle_cd(int client_fd, const char *pathname);
// 列出目录文件
void handle_ls(int client_fd);
// 上传文件
void handle_puts(int client_fd, const char *filename);
// 下载文件
void handle_gets(int client_fd, const char *filename);
// 删除文件
void handle_remove(int client_fd, const char *filename);
// 打印当前工作目录
void handle_pwd(int client_fd);
// 创建文件夹
void handle_mkdir(int client_fd, const char *dirname);

#endif // FILE_OPERATIONS_H

