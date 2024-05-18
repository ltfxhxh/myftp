#include "server.h"
#include "network_utils.h"
#include "epoll_manager.h"
#include "thread_pool.h"
#include "command_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void server_init_and_run(const char *ip, int port, int min_threads,
                         int max_threads, int queue_size) {
    int server_fd = create_server_socket(ip, port);
    if (server_fd < 0) {
        fprintf(stderr, "Error setting up server socket\n");
        return;
    }

    ThreadPool *pool = thread_pool_create(min_threads, max_threads, queue_size);
    if (pool == NULL) {
        fprintf(stderr, "Error creating thread pool\n");
        close(server_fd);
        return;
    }

    pthread_t monitor_thread;
    if (pthread_create(&monitor_thread, NULL, monitor_task_queue, (void *)pool) != 0) {
        perror("Failed to create monitor thread");
        thread_pool_destroy(pool, 0);
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
    pthread_join(monitor_thread, NULL);
}

// 固定步长策略调整线程池
void *monitor_task_queue(void *arg) {
    ThreadPool *pool = (ThreadPool *)arg;
    const int step = 2;
    
    while (1) {
        sleep(120);
        pthread_mutex_lock(&(pool->lock));

        if (pool->shutdown) {
            pthread_mutex_unlock(&(pool->lock));
            break;
        }

        int new_size;
        if (pool->count > pool->queue_size / 2 && pool->thread_count 
            < pool->max_threads) {
            new_size = pool->thread_count + step;
            if (new_size > pool->max_threads) {
                new_size = pool->max_threads;
            }
        } else if (pool->count < pool->queue_size / 4 && pool->thread_count 
                   > pool->min_threads) {
            new_size = pool->thread_count -step;
            if (new_size < pool->min_threads) {
                new_size = pool->min_threads;
            }
        } else {
            pthread_mutex_unlock(&(pool->lock));

            thread_pool_resize(pool, new_size);
        }
    }

    return NULL;
}
