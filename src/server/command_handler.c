#include "command_handler.h"
#include "auth_handler.h"
#include "file_operations.h"
#include "logger.h"
#include "network_utils.h"
#include "jwt_util.h"
#include "config.h"
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <json-c/json.h>

void handle_input(int client_fd) {
    char buffer[BUFSIZ];
    ssize_t numbytes;

    memset(buffer, 0, sizeof(buffer));

    numbytes = read(client_fd, buffer, sizeof(buffer));
    if (numbytes < 0) {
        perror("read error");
        LOG_ERROR("Error reading from client FD=%d: %s", client_fd, strerror(errno));
        close(client_fd);
        return;
    } else if (numbytes == 0) {
        LOG_ERROR("Client disconnected, FD=%d", client_fd);
        printf("Client disconnected\n");
        close(client_fd);
        return;
    }

    process_command(client_fd, buffer);
}

void process_command(int client_fd, const char *command) {
    LOG_INFO("Processing command from FD=%d: %s", client_fd, command);

    json_object *parsed_command = json_tokener_parse(command);
    if (parsed_command == NULL) {
        LOG_ERROR("Failed to parse command as JSON from FD=%d", client_fd);
        return;
    }

    const char *cmd = json_object_get_string(json_object_object_get(parsed_command, "command"));
    json_object *arg = json_object_object_get(parsed_command, "args");
    const char *args = json_object_get_string(json_object_object_get(parsed_command, "args"));
    const char *token = json_object_get_string(json_object_object_get(parsed_command, "token"));

    if (cmd == NULL) {
        LOG_ERROR("Invalid command from FD=%d", client_fd);
        json_object_put(parsed_command);
        return;
    }

    int user_id = -1;
    if (strcmp(cmd, "register") != 0 && strcmp(cmd, "login") != 0) {
        if (token == NULL || (user_id = extract_user_id_from_token(token)) == -1) {
            LOG_ERROR("Invalid token from FD=%d", client_fd);
            json_object_put(parsed_command);
            return;
        }
    }

    MYSQL *conn = db_connect();
    if (conn == NULL) {
        LOG_ERROR("Database connection failed for FD=%d", client_fd);
        json_object_put(parsed_command);
        return;
    }

    if (strcmp(cmd, "register") == 0) {
        handle_register_request(client_fd, arg);
    } else if (strcmp(cmd, "login") == 0) {
        handle_login_request(client_fd, arg);
    } else if (strcmp(cmd, "cd") == 0) {
        handle_cd(client_fd, args, user_id, conn);
    } else if (strcmp(cmd, "ls") == 0) {
        handle_ls(client_fd, user_id, conn);
    } else if (strcmp(cmd, "puts") == 0) {
        handle_puts(client_fd, args, user_id, conn);
    } else if (strcmp(cmd, "gets") == 0) {
        handle_gets(client_fd, args, user_id, conn);
    } else if (strcmp(cmd, "rm") == 0) {
        handle_remove(client_fd, args, user_id, conn);
    } else if (strcmp(cmd, "pwd") == 0) {
        handle_pwd(client_fd, user_id, conn);
    } else if (strcmp(cmd, "mkdir") == 0) {
        handle_mkdir(client_fd, args, user_id, conn);
    }

    json_object_put(parsed_command);
    db_disconnect(conn);
}
