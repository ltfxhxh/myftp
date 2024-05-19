#ifndef SERVER_H
#define SERVER_H

#include "network_utils.h"
#include "thread_pool.h"
#include "command_handler.h"
#include "epoll_manager.h"

// 初始化并启动服务器
void server_init_and_run(const char *ip, int port, int min_threads, int max_threads, int queue_size);

// 监控任务队列,调整线程池大小
void *monitor_task_queue(void *arg);

#endif // SERVER_H

