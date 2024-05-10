#include "command_handler.h"
#include "file_operations.h"
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

/*========== 处理命令行输入 ==========*/
void handle_input(int client_fd) {
    char buffer[BUFSIZ];
    ssize_t numbytes;

    memset(buffer, 0, sizeof(buffer));

    // 从客户端读取数据
    numbytes = read(client_fd, buffer, sizeof(buffer));
    if (numbytes < 0) {
        perror("read error");
        exit(EXIT_FAILURE);
    } else if (numbytes == 0) {
        printf("Client disconnected\n");
        close(client_fd);
        return;
    }

    process_command(client_fd, buffer);
}

/*========== 处理命令行输入的命令 ==========*/
void process_command(int client_fd, const char *command) {
    char *args = strdup(command);
    if (!args) {
        perror("strdup failed");
        exit(EXIT_FAILURE);
    }

    char *saveptr;
    char *cmd = strtok_r(args, " \t\r\n", &saveptr);

    if (cmd == NULL) {
        handle_invalid(client_fd);
    } else if (strcmp(cmd, "exit") == 0) {
        const char *msg = "Exiting, goodbye!\n";
        write(client_fd, msg, strlen(msg));
        close(client_fd);  // Close the connection when "exit" command is received
    } else if (strcmp(cmd, "cd") == 0) {
        handle_cd(client_fd, strtok_r(NULL, " \t\r\n", &saveptr));
    } else if (strcmp(cmd, "ls") == 0) {
        handle_ls(client_fd);
    } else if (strcmp(cmd, "puts") == 0) {
        handle_puts(client_fd, strtok_r(NULL, " \t\r\n", &saveptr));
    } else if (strcmp(cmd, "gets") == 0) {
        handle_gets(client_fd, strtok_r(NULL, " \t\r\n", &saveptr));
    } else if (strcmp(cmd, "remove") == 0) {
        handle_remove(client_fd, strtok_r(NULL, " \t\r\n", &saveptr));
    } else if (strcmp(cmd, "pwd") == 0) {
        handle_pwd(client_fd);
    } else if (strcmp(cmd, "mkdir") == 0) {
        handle_mkdir(client_fd, strtok_r(NULL, " \t\r\n", &saveptr));
    } else {
        handle_invalid(client_fd);
    }

    free(args);
}

/*========== 处理命令行输入的无效命令 ==========*/
void handle_invalid(int client_fd) {
    const char *msg = "无效的或不支持的命令\n";
    write(client_fd, msg, strlen(msg));
}
