// client_main.c
#include "network.h"
#include "ui.h"
#include "utils.h"
#include "login.h"
#include "logger.h"
#include "file_transfer.h"
#include <json-c/json.h>

#define BUFFER_SIZE 1024

const char *END_OF_MESSAGE = "\n.\n";
char token[BUFFER_SIZE];
char current_path[1024] = "/";
char usr[BUFFER_SIZE];

void send_json_command(int sockfd, const char *command, const char *args) {
    struct json_object *json_obj = json_object_new_object();
    json_object_object_add(json_obj, "command", json_object_new_string(command));
    if (args) {
        json_object_object_add(json_obj, "args", json_object_new_string(args));
    }
    if (strlen(token) > 0) {
        LOG_DEBUG("加入token:%s", token);
        json_object_object_add(json_obj, "token", json_object_new_string(token));
    }

    const char *json_str = json_object_to_json_string(json_obj);
    send(sockfd, json_str, strlen(json_str), 0);
    json_object_put(json_obj);
}

int main(int argc, char *argv[]) {
    // 初始化日志系统
    log_init("../logs/client.log");
    LOG_INFO("client starting...");
    
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <server IP> <server port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    int sockfd = connect_to_server(server_ip, server_port);
    if (sockfd < 0) {
        return EXIT_FAILURE;
    }

    while (1) {
        display_login();

        int choice;
        printf("输入选项: ");
        scanf("%d", &choice);
        getchar();

        if (choice == 1) {
            // 调用登录函数
            if (handle_login(sockfd, token, usr)) {
                break;  // 登陆成功,跳出循环,进入命令行操作模式
            }
        } else if (choice == 2) {
            // 调用注册函数
            handle_register(sockfd);
        } else if (choice == 3) {
            // 退出程序
            printf(ANSI_COLOR_GREEN "退出中...\n" ANSI_COLOR_RESET);
            close(sockfd);
            return EXIT_SUCCESS;
        } else {
            printf(ANSI_COLOR_RED "无效的选项,请重新输入.\n" ANSI_COLOR_RESET);
        }
    }

    display_welcome_message();

    char command[BUFFER_SIZE];
    while (1) {
        printf(ANSI_COLOR_YELLOW "%s@%s$ " ANSI_COLOR_RESET, usr, current_path);
        fflush(stdout);

        if (fgets(command, BUFFER_SIZE, stdin) == NULL) {
            printf(ANSI_COLOR_RED "Failed to read command.\n" ANSI_COLOR_RESET);
            continue;
        }

        trim_newline(command);

        if (strlen(command) == 0) {
            continue;
        }

        if (strcmp(command, "exit") == 0) {
            printf(ANSI_COLOR_GREEN "Exiting...\n" ANSI_COLOR_RESET);
            break;
        }

        char *backup = strdup(command);
        char *space = strchr(command, ' ');
        if (space != NULL) {
            *space = '\0';
            send_json_command(sockfd, command, space + 1);
        } else {
            send_json_command(sockfd, command, NULL);
        }

        LOG_INFO("发送命令:%s", command);
        
        process_command(sockfd, backup);
        free(backup); // 释放内存
    }

    close(sockfd);

    LOG_INFO("client shut down.");
    log_close();

    return EXIT_SUCCESS;
}

