// file_transfer.h
#ifndef FILE_TRANSFER_H
#define FILE_TRANSFER_H

#include "common.h"

void process_command(int sockfd, const char *command);

// 处理 "gets" 命令
void handle_gets(int sockfd, const char *local_filename);

// 处理 "puts" 命令
void handle_puts(int sockfd, const char *local_filename);

// 接收完整响应函数
ssize_t recv_full_response(int server_fd, char *buffer, size_t buffer_size);

// pwd命令响应处理函数
void handle_pwd_response(int server_fd);

// cd命令响应处理函数
void handle_cd_response(int server_fd);

// ls命令响应处理函数
void handle_ls_response(int server_fd);

// mkdir命令响应处理函数
void handle_mkdir_response(int server_fd);

// remove命令响应处理函数
void handle_remove_response(int server_fd);

#endif // FILE_TRANSFER_H

