// ui.c
#include "ui.h"

void display_login() {
    printf(ANSI_COLOR_CYAN ANSI_BOLD_ON "\n欢迎来到个人服务器\n" ANSI_BOLD_OFF ANSI_COLOR_RESET);
    printf(ANSI_COLOR_YELLOW "请选择一个选项:\n" ANSI_COLOR_RESET);
    printf(ANSI_COLOR_GREEN "1. 登录\n" ANSI_COLOR_RESET);
    printf(ANSI_COLOR_GREEN "2. 注册\n" ANSI_COLOR_RESET);
    printf(ANSI_COLOR_GREEN "3. 退出\n" ANSI_COLOR_RESET);
}


void display_welcome_message() {
    printf(ANSI_COLOR_CYAN ANSI_BOLD_ON "\n欢迎来到个人服务器\n\n" ANSI_BOLD_OFF ANSI_COLOR_RESET);
    printf(ANSI_COLOR_YELLOW "请保证您的操作命令在下列命令列表中 (输入'exit'以退出):\n" ANSI_COLOR_RESET);
    printf(ANSI_COLOR_GREEN "--------------------------------------------------------------------\n" ANSI_COLOR_RESET);
    printf(ANSI_COLOR_MAGENTA "ls" ANSI_COLOR_RESET "                                           - 列出文件列表\n");
    printf(ANSI_COLOR_MAGENTA "cd <path>" ANSI_COLOR_RESET "                                    - 修改当前工作目录\n");
    printf(ANSI_COLOR_MAGENTA "gets <server_filename> <local_filename>" ANSI_COLOR_RESET "      - 从服务器下载文件\n");
    printf(ANSI_COLOR_MAGENTA "puts <local_filename> <server_filename>" ANSI_COLOR_RESET "      - 上传文件至服务器\n");
    printf(ANSI_COLOR_MAGENTA "remove <filename>" ANSI_COLOR_RESET "                            - 删除文件或空目录\n");
    printf(ANSI_COLOR_MAGENTA "pwd" ANSI_COLOR_RESET "                                          - 打印当前工作目录\n");
    printf(ANSI_COLOR_MAGENTA "mkdir <directory>" ANSI_COLOR_RESET "                            - 创建空目录\n");
    printf(ANSI_COLOR_GREEN "--------------------------------------------------------------------\n" ANSI_COLOR_RESET);
}

void trim_newline(char *str) {
    int len = strlen(str);
    if (len > 0 && str[len - 1] == '\n') {
        str[len - 1] = '\0';
    }
}

