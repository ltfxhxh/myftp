#include "network_utils.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>

int create_server_socket(const char *ip, int port) {
    LOG_INFO("Attempting to create server socket at %s:%d", ip, port);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1) {
        //perror("Failed to create socket");
        LOG_ERROR("Failed to create socket: %s", strerror(errno));
        return -1;
    }
    LOG_DEBUG("Socket created, FD=%d", sockfd);

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0) {
        LOG_ERROR("Failed to set socket options: %s", strerror(errno));
        close(sockfd);
        return -1;
    }
    LOG_DEBUG("Socket options set, SO_REUSEADDR");

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);
    
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        // perror("Failed to bind");
        LOG_ERROR("Failed to bind socket to %s:%d: %s", ip, port, strerror(errno));
        close(sockfd);
        return -1;
    }
    LOG_DEBUG("Socket bound to %s:%d", ip, port);

    if (listen(sockfd, SOMAXCONN) < 0) {
        // perror("Failed to listen");
        LOG_ERROR("Failed to listen on socket: %s", strerror(errno));
        close(sockfd);
        return -1;
    }
    LOG_INFO("Server socket listening on %s:%d", ip, port);

    return sockfd;
}
