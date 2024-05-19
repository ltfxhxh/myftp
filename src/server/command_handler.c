#include "command_handler.h"
#include "file_operations.h"
#include "logger.h"
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

/*========== 处理命令行输入 ==========*/
void handle_input(int client_fd) {
    char buffer[BUFSIZ];
    ssize_t numbytes;

    memset(buffer, 0, sizeof(buffer));

    // 从客户端读取数据
    numbytes = read(client_fd, buffer, sizeof(buffer));
    if (numbytes < 0) {
        perror("read error");
        LOG_ERROR("Error reading from client FD=%d: %s", client_fd, strerror(errno));
        exit(EXIT_FAILURE);
    } else if (numbytes == 0) {
        LOG_INFO("Client disconnected, FD=%d", client_fd);
        printf("Client disconnected\n");
        close(client_fd);
        return;
    }

    LOG_DEBUG("Received command from FD=%d: %s", client_fd, buffer);
    process_command(client_fd, buffer);
}

/*========== 处理命令行输入的命令 ==========*/
void process_command(int client_fd, const char *command) {
    LOG_INFO("Processing command from FD=%d: %s", client_fd, command);

    char *args = strdup(command);
    if (!args) {
        LOG_ERROR("Memory allocation failed when duplicating command string.");
        perror("strdup failed");
        exit(EXIT_FAILURE);
    }

    char *saveptr;
    char *cmd = strtok_r(args, " \t\r\n", &saveptr);
    char *path;

    if (cmd == NULL) {
        LOG_WARNING("Empty command from FD=%d", client_fd);
        handle_invalid(client_fd);
    } else if (strcmp(cmd, "exit") == 0) {
        LOG_INFO("Exit command received from FD=%d", client_fd);
        const char *msg = "Exiting, goodbye!\n";
        write(client_fd, msg, strlen(msg));
        close(client_fd);  // Close the connection when "exit" command is received
    } else if (strcmp(cmd, "cd") == 0) {
        path = strtok_r(NULL, " \t\r\n", &saveptr);
        LOG_DEBUG("CD command with path=%s from FD=%d", path, client_fd);
        handle_cd(client_fd, path);
    } else if (strcmp(cmd, "ls") == 0) {
        LOG_DEBUG("LS command received from FD=%d", client_fd);
        handle_ls(client_fd);
    } else if (strcmp(cmd, "puts") == 0) {
        path = strtok_r(NULL, " \t\r\n", &saveptr);
        LOG_DEBUG("PUTS command with filename=%s from FD=%d", path, client_fd);
        handle_puts(client_fd, path);
    } else if (strcmp(cmd, "gets") == 0) {
        path = strtok_r(NULL, " \t\r\n", &saveptr);
        LOG_DEBUG("GETS command with filename=%s from FD=%d", path, client_fd);
        handle_gets(client_fd, path);
    } else if (strcmp(cmd, "remove") == 0) {
        path = strtok_r(NULL, " \t\r\n", &saveptr);
        LOG_DEBUG("REMOVE command with filename=%s from FD=%d", path, client_fd);
        handle_remove(client_fd, path);
    } else if (strcmp(cmd, "pwd") == 0) {
        LOG_DEBUG("PWD command received from FD=%d", client_fd);
        handle_pwd(client_fd);
    } else if (strcmp(cmd, "mkdir") == 0) {
        path = strtok_r(NULL, " \t\r\n", &saveptr);
        LOG_DEBUG("MKDIR command with dirname=%s from FD=%d", path, client_fd);
        handle_mkdir(client_fd, path);
    } else {
        LOG_WARNING("Invalid command '%s' from FD=%d", cmd, client_fd);
        handle_invalid(client_fd);
    }

    free(args);
}

/*========== 处理命令行输入的无效命令 ==========*/
void handle_invalid(int client_fd) {
    LOG_WARNING("Handling invalid command for FD=%d", client_fd);
    const char *msg = "无效的或不支持的命令\n";
    write(client_fd, msg, strlen(msg));
}
