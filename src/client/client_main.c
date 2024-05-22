// client_main.c
#include "network.h"
#include "ui.h"
#include "utils.h"
#include "login.h"
#include "logger.h"

const char *END_OF_MESSAGE = "\n.\n";

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
            if (handle_login(sockfd)) {
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
    char cwd[BUFFER_SIZE];
    while (1) {
        bzero(cwd, BUFFER_SIZE);
        check_cwd(cwd, BUFFER_SIZE);

        printf(ANSI_COLOR_YELLOW "~%s$ " ANSI_COLOR_RESET, cwd);
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

        LOG_INFO("发送命令:%s", command);
       send_command(sockfd, command);
    }

    close(sockfd);

    LOG_INFO("client shut down.");
    log_close();

    return EXIT_SUCCESS;
}

