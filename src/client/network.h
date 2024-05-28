// network.h
#ifndef NETWORK_H
#define NETWORK_H

#include "common.h"

int set_nonblocking(int sockfd);
int connect_to_server(const char *server_ip, int server_port);

#endif // NETWORK_H

