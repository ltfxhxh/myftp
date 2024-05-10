#ifndef NETWORKING_H
#define NETWORKING_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "thread_pool.h"  // 包含线程池的定义

// 创建服务端socket,包含bind和listen
int create_server_socket(const char *ip, int port);

#endif // NETWORKING_H

