#include "epoll_manager.h"
#include "command_handler.h"
#include "logger.h"  // Include the logger header
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// 创建一个新的 epoll 实例
int epoll_create_instance(void) {
    LOG_DEBUG("Attempting to create a new epoll instance.");
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        LOG_ERROR("Failed to create epoll instance. Error: %s", strerror(errno));
        perror("epoll_create1 failed");
    } else {
        LOG_INFO("Created new epoll instance with FD: %d", epoll_fd);
    }
    return epoll_fd;
}

// 向 epoll 实例添加文件描述符
void epoll_add_fd(int epoll_fd, int fd, struct epoll_event *event) {
    LOG_DEBUG("Adding FD %d to epoll instance with FD %d.", fd, epoll_fd);
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, event) == -1) {
        LOG_ERROR("Failed to add FD %d to epoll instance. Error: %s", fd, strerror(errno));
        perror("epoll_ctl add fd failed");
    } else {
        LOG_INFO("FD %d added to epoll instance.", fd);
    }
}

// 从 epoll 实例删除文件描述符
void epoll_remove_fd(int epoll_fd, int fd) {
    LOG_DEBUG("Removing FD %d from epoll instance with FD %d.", fd, epoll_fd);
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
        LOG_ERROR("Failed to remove FD %d from epoll instance. Error: %s", fd, strerror(errno));
        perror("epoll_ctl remove fd failed");
    } else {
        LOG_INFO("FD %d removed from epoll instance.", fd);
    }
}

// 修改 epoll 实例中的文件描述符
void epoll_modify_fd(int epoll_fd, int fd, struct epoll_event *event) {
    LOG_DEBUG("Modifying FD %d on epoll instance with FD %d.", fd, epoll_fd);
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, event) == -1) {
        LOG_ERROR("Failed to modify FD %d on epoll instance. Error: %s", fd, strerror(errno));
        perror("epoll_ctl modify fd failed");
    } else {
        LOG_INFO("FD %d modified on epoll instance.", fd);
    }
}

// 开始处理 epoll 事件循环
void start_epoll_loop(int epoll_fd, ThreadPool *pool, int server_fd) {
    struct epoll_event events[MAX_EVENTS];
    int i, num_fds;

    LOG_INFO("Starting epoll event loop.");
    while (1) {
        num_fds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (num_fds == -1) {
            LOG_ERROR("Epoll wait failed. Error: %s", strerror(errno));
            perror("epoll_wait failed");
            continue;
        }

        LOG_DEBUG("Processing %d events.", num_fds);
        for (i = 0; i < num_fds; i++) {
            if (events[i].data.fd == server_fd) {
                struct sockaddr_in client_addr;
                socklen_t client_addr_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
                if (client_fd == -1) {
                    LOG_ERROR("Failed to accept client connection. Error: %s", strerror(errno));
                    perror("Failed to accept client connection");
                    continue;
                }

                struct epoll_event client_event;
                client_event.data.fd = client_fd;
                client_event.events = EPOLLIN | EPOLLET;
                epoll_add_fd(epoll_fd, client_fd, &client_event);
                LOG_INFO("Accepted new connection: FD %d", client_fd);
            } else {
                if (events[i].events & EPOLLIN) {
                    int fd = events[i].data.fd;
                    LOG_DEBUG("Adding task for FD %d to thread pool.", fd);
                    thread_pool_add(pool, (void (*)(void *))handle_input, (void *)(intptr_t)fd);
                } else if (events[i].events & (EPOLLHUP | EPOLLERR)) {
                    LOG_INFO("Closing FD %d due to HUP or ERR.", events[i].data.fd);
                    close(events[i].data.fd);
                    epoll_remove_fd(epoll_fd, events[i].data.fd);
                }
            }
        }
    }
}
