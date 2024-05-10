#include "epoll_manager.h"
#include "command_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// 创建一个新的 epoll 实例
int epoll_create_instance(void) {
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1 failed");
    }
    return epoll_fd;
}

// 向 epoll 实例添加文件描述符
void epoll_add_fd(int epoll_fd, int fd, struct epoll_event *event) {
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, event) == -1) {
        perror("epoll_ctl add fd failed");
    }
}

// 从 epoll 实例删除文件描述符
void epoll_remove_fd(int epoll_fd, int fd) {
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
        perror("epoll_ctl remove fd failed");
    }
}

// 修改 epoll 实例中的文件描述符
void epoll_modify_fd(int epoll_fd, int fd, struct epoll_event *event) {
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, event) == -1) {
        perror("epoll_ctl modify fd failed");
    }
}

// 开始处理 epoll 事件循环
void start_epoll_loop(int epoll_fd, ThreadPool *pool, int server_fd) {
    struct epoll_event events[MAX_EVENTS];
    int i, num_fds;

    while (1) {
        num_fds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (num_fds == -1) {
            perror("epoll_wait failed");
            continue;
        }

        for (i = 0; i < num_fds; i++) {
            if (events[i].data.fd == server_fd) {
                struct sockaddr_in client_addr;
                socklen_t client_addr_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
                if (client_fd == -1) {
                    perror("Failed to accept client connection");
                    continue;
                }

                struct epoll_event client_event;
                client_event.data.fd = client_fd;
                client_event.events = EPOLLIN | EPOLLET;
                epoll_add_fd(epoll_fd, client_fd, &client_event);
                printf("Accepted new connection: %d\n", client_fd);
            } else {
                if (events[i].events & EPOLLIN) {
                    int fd = events[i].data.fd;
                    thread_pool_add(pool, (void (*)(void *))handle_input, (void *)(intptr_t)fd);
                } else if (events[i].events & (EPOLLHUP | EPOLLERR)) {
                    close(events[i].data.fd);
                    epoll_remove_fd(epoll_fd, events[i].data.fd);
                }
            }
        }
    }
}

