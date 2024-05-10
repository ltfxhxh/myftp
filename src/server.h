#ifndef SERVER_H
#define SERVER_H

#include "network_utils.h"
#include "thread_pool.h"
#include "command_handler.h"
#include "epoll_manager.h"
#include "reactor.h"

// 初始化并启动服务器
void server_init_and_run(const char *ip, int port, int thread_pool_size);

#endif // SERVER_H

