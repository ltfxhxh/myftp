#include "server.h"
#include "network_utils.h"
#include "epoll_manager.h"
#include "thread_pool.h"
#include "command_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void server_init_and_run(const char *ip, int port, int thread_pool_size) {
    int server_fd = create_server_socket(ip, port);
    if (server_fd < 0) {
        fprintf(stderr, "Error setting up server socket\n");
        return;
    }

    ThreadPool *pool = thread_pool_create(thread_pool_size, MAX_EVENTS);
    if (pool == NULL) {
        fprintf(stderr, "Error creating thread pool\n");
        close(server_fd);
        return;
    }

    int epoll_fd = epoll_create_instance();
    if (epoll_fd < 0) {
        fprintf(stderr, "Error creating epoll instance\n");
        thread_pool_destroy(pool, 0);
        close(server_fd);
        return;
    }

    struct epoll_event event;
    event.data.fd = server_fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_add_fd(epoll_fd, server_fd, &event);

    printf("Server listening on %s:%d\n", ip, port);
    start_epoll_loop(epoll_fd, pool, server_fd);

    close(server_fd);
    thread_pool_destroy(pool, 1);
    close(epoll_fd);
}

