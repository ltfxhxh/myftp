// network.c
#include "network.h"
#include "utils.h"
#include "file_transfer.h"

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

int connect_to_server(const char *server_ip, int server_port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror(ANSI_COLOR_RED "Failed to create socket" ANSI_COLOR_RESET);
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror(ANSI_COLOR_RED "Failed to connect to the server" ANSI_COLOR_RESET);
        close(sockfd);
        return -1;
    }

    return sockfd;
}

