#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>      // 提供 fcntl 函数
#include <errno.h>      // 提供 errno 变量
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

// ANSI Color Codes for output
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_BOLD_ON       "\x1b[1m"
#define ANSI_BOLD_OFF      "\x1b[22m"

const char *END_OF_MESSAGE = "\n.\n";

// Function to set socket as non-blocking
int set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("Error getting flags on socket");
        return -1;
    }

    flags |= O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, flags) == -1) {
        perror("Error setting non-blocking flag on socket");
        return -1;
    }

    return 0;
}

void display_welcome_message() {
    printf(ANSI_COLOR_CYAN ANSI_BOLD_ON "\n欢迎来到个人服务器\n\n" ANSI_BOLD_OFF ANSI_COLOR_RESET);
    printf(ANSI_COLOR_YELLOW "请保证您的操作命令在下列命令列表中 (输入'exit'以退出):\n" ANSI_COLOR_RESET);
    printf(ANSI_COLOR_GREEN "------------------------------------------------\n" ANSI_COLOR_RESET);
    printf(ANSI_COLOR_MAGENTA "ls" ANSI_COLOR_RESET "      - 列出文件列表\n");
    printf(ANSI_COLOR_MAGENTA "cd <path>" ANSI_COLOR_RESET " - 修改当前工作目录\n");
    printf(ANSI_COLOR_MAGENTA "gets <server_filename> <local_filename>" ANSI_COLOR_RESET " - 从服务器下载文件\n");
    printf(ANSI_COLOR_MAGENTA "puts <local_filename> <server_filename>" ANSI_COLOR_RESET " - 上传文件至服务器\n");
    printf(ANSI_COLOR_MAGENTA "remove <filename>" ANSI_COLOR_RESET " - 删除文件或空目录\n");
    printf(ANSI_COLOR_MAGENTA "pwd" ANSI_COLOR_RESET "     - 打印当前工作目录\n");
    printf(ANSI_COLOR_MAGENTA "mkdir <directory>" ANSI_COLOR_RESET " - 创建空目录\n");
    printf(ANSI_COLOR_GREEN "------------------------------------------------\n" ANSI_COLOR_RESET);
}

void trim_newline(char *str) {
    int len = strlen(str);
    if (len > 0 && str[len - 1] == '\n') {
        str[len - 1] = '\0';
    }
}

void send_command(int sockfd, const char* command) {
    char buffer[BUFFER_SIZE] = {0};

    if (send(sockfd, command, strlen(command), 0) < 0) {
        perror(ANSI_COLOR_RED "Failed to send data to server" ANSI_COLOR_RESET);
        return;
    }

    if (set_nonblocking(sockfd) == -1) {
        printf(ANSI_COLOR_RED "Failed to set non-blocking socket mode" ANSI_COLOR_RESET);
        return;
    }

    char cmd[100], local_filename[256], remote_filename[256];
    int cmd_result = sscanf(command, "%s %s %s", cmd, local_filename, remote_filename);

    if (strcmp(cmd, "gets") == 0 && cmd_result == 3) {
        int file_fd = open(local_filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (file_fd < 0) {
            perror("Error opening local file for writing");
            return;
        }

        ssize_t received;
        int total_received = 0;
        while (1) {
            received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
            if (received > 0) {
                buffer[received] = '\0';
                if (strstr(buffer, END_OF_MESSAGE)) {
                    *strstr(buffer, END_OF_MESSAGE) = '\0';
                    if (write(file_fd, buffer, strlen(buffer)) != (ssize_t)strlen(buffer)) {
                        perror("Error writing last part to local file");
                    }
                    break;
                }
                if (write(file_fd, buffer, received) != received) {
                    perror("Error writing to local file");
                    close(file_fd);
                    return;
                }
                total_received += received;
            } else if (received == 0) {
                break;
            } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("recv failed");
                break;
            }
        }
        printf(ANSI_COLOR_GREEN "Received %d bytes and saved to '%s'.\n" ANSI_COLOR_RESET, total_received, local_filename);
        close(file_fd);
    } else if (strcmp(cmd, "puts") == 0 && cmd_result == 3) {
        int file_fd = open(local_filename, O_RDONLY);
        if (file_fd < 0) {
            perror("Error opening local file for reading");
            return;
        }

        ssize_t read_bytes;
        int total_sent = 0;
        while ((read_bytes = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
            if (send(sockfd, buffer, read_bytes, 0) != read_bytes) {
                perror("Error sending file data to server");
                close(file_fd);
                return;
            }
            total_sent += read_bytes;
        }
        if (read_bytes < 0) {
            perror("Error reading local file");
        } else {
            printf(ANSI_COLOR_GREEN "Sent %d bytes from '%s' to server.\n" ANSI_COLOR_RESET, total_sent, local_filename);
        }
        close(file_fd);
    } else {
        ssize_t received;
        int total_received = 0;
        while (1) {
            received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
            if (received > 0) {
                buffer[received] = '\0';
                if (strstr(buffer, END_OF_MESSAGE)) {
                    *strstr(buffer, END_OF_MESSAGE) = '\0';
                    printf(ANSI_COLOR_BLUE "%s" ANSI_COLOR_RESET, buffer);
                    break;
                }
                printf(ANSI_COLOR_BLUE "%s" ANSI_COLOR_RESET, buffer);
                total_received += received;
            } else if (received == 0) {
                break;
            } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("recv failed");
                break;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <server IP> <server port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror(ANSI_COLOR_RED "Failed to create socket" ANSI_COLOR_RESET);
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror(ANSI_COLOR_RED "Failed to connect to the server" ANSI_COLOR_RESET);
        close(sockfd);
        return EXIT_FAILURE;
    }

    display_welcome_message();

    char command[BUFFER_SIZE];
    char cwd[BUFFER_SIZE];
    while (1) {
        bzero(cwd, BUFFER_SIZE);
        if ((getcwd(cwd, BUFFER_SIZE)) == NULL) {
            perror("getcwd error");
            return -1;
        }
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
            printf(ANSI_COLOR_GREEN "退出中...\n" ANSI_COLOR_RESET);
            break;
        }

        send_command(sockfd, command);
    }

    close(sockfd);
    return EXIT_SUCCESS;
}

