#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <sys/types.h>
#include <stddef.h>

// 处理客户端输入
void handle_input(int client_fd);
void process_command(int client_fd, const char *command);


/*========== 扩展命令处理 ==========*/
// 处理无效命令
void handle_invalid(int client_fd);

#endif
