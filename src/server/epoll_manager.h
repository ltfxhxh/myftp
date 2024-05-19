#ifndef EPOLL_MANAGER_H
#define EPOLL_MANAGER_H

#include <sys/epoll.h>
#include <pthread.h>
#include "thread_pool.h"

// 定义最大的事件数量
#define MAX_EVENTS 1024

// 创建 epoll 实例
int epoll_create_instance(void);

// 向 epoll 实例添加文件描述符
void epoll_add_fd(int epoll_fd, int fd, struct epoll_event *event);

// 从 epoll 实例删除文件描述符
void epoll_remove_fd(int epoll_fd, int fd);

// 修改 epoll 实例中的文件描述符
void epoll_modify_fd(int epoll_fd, int fd, struct epoll_event *event);

// 开始处理 epoll 事件循环
void start_epoll_loop(int epoll_fd, ThreadPool *pool, int server_fd);

#endif // EPOLL_MANAGER_H

