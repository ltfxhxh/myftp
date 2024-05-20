#include "server.h"
#include "network_utils.h"
#include "epoll_manager.h"
#include "thread_pool.h"
#include "command_handler.h"
#include "logger.h"
#include "auth_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void server_init_and_run(const char *ip, int port, int min_threads,
                         int max_threads, int queue_size) {
    LOG_INFO("Initializing server, IP=%s, Port=%d, MinThreads=%d, MaxThreads=%d, QueueSize=%d", 
             ip, port, min_threads, max_threads, queue_size);
    int server_fd = create_server_socket(ip, port);
    if (server_fd < 0) {
        LOG_ERROR("Failed to create server socket");
        // fprintf(stderr, "Error setting up server socket\n");
        return;
    }
    LOG_INFO("Server socket created, FD=%d", server_fd);

    ThreadPool *pool = thread_pool_create(min_threads, max_threads, queue_size);
    if (pool == NULL) {
        // fprintf(stderr, "Error creating thread pool\n");
        LOG_ERROR("Failed to create thread pool.");
        close(server_fd);
        return;
    }
    LOG_INFO("Thread pool created, MinThreads=%d, MaxThreads=%d, QueueSize=%d", 
             min_threads, max_threads, queue_size);

    pthread_t monitor_thread;
    if (pthread_create(&monitor_thread, NULL, monitor_task_queue, (void *)pool) != 0) {
        // perror("Failed to create monitor thread");
        LOG_ERROR("Failed to create monitor thread.");
        thread_pool_destroy(pool, 0);
        close(server_fd);
        return;
    }
    LOG_INFO("Monitor thread created.");

    int epoll_fd = epoll_create_instance();
    if (epoll_fd < 0) {
        // fprintf(stderr, "Error creating epoll instance\n");
        LOG_ERROR("Failed to create epoll instance.");
        thread_pool_destroy(pool, 0);
        close(server_fd);
        return;
    }
    LOG_INFO("Epoll instance created, FD=%d", epoll_fd);

    struct epoll_event event;
    event.data.fd = server_fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_add_fd(epoll_fd, server_fd, &event);
    LOG_INFO("Server socket added to epoll, FD=%d", server_fd);

    LOG_INFO("Server is now listening on %s:%d", ip, port);
    // printf("Server listening on %s:%d\n", ip, port);
    start_epoll_loop(epoll_fd, pool, server_fd);

    close(server_fd);
    LOG_INFO("Server socket closed, FD=%d", server_fd);

    thread_pool_destroy(pool, 1);
    LOG_INFO("Thread pool destroyed");
    
    close(epoll_fd);
    LOG_INFO("Epoll instance closed, FD=%d", epoll_fd);
    
    pthread_join(monitor_thread, NULL);
    LOG_INFO("Monitor thread joined");

    LOG_INFO("Server has shut down.");
}

// 固定步长策略调整线程池
void *monitor_task_queue(void *arg) {
    ThreadPool *pool = (ThreadPool *)arg;
    const int step = 2;
    
    LOG_INFO("Monitor thread is monitoring thread pool");
    while (1) {
        sleep(120);
        pthread_mutex_lock(&(pool->lock));

        if (pool->shutdown) {
            pthread_mutex_unlock(&(pool->lock));
            LOG_INFO("Monitor thread detected shutdown signal");
            break;
        }

        int new_size;
        if (pool->count > pool->queue_size / 2 && pool->thread_count 
            < pool->max_threads) {
            new_size = pool->thread_count + step;
            if (new_size > pool->max_threads) {
                new_size = pool->max_threads;
            }
            LOG_INFO("Increasing thread pool size to %d", new_size);
        } else if (pool->count < pool->queue_size / 4 && pool->thread_count 
                   > pool->min_threads) {
            new_size = pool->thread_count -step;
            if (new_size < pool->min_threads) {
                new_size = pool->min_threads;
            }
            LOG_INFO("Decreasing thread pool size to %d", new_size);
        } else {
            pthread_mutex_unlock(&(pool->lock));
            continue;
        }

        if (thread_pool_resize(pool, new_size) == 0) {
            LOG_INFO("Thread pool resized to %d", new_size);
        } else {
            LOG_ERROR("Failed to resize thread pool to %d", new_size);
        }

        pthread_mutex_unlock(&(pool->lock));
    }
    LOG_INFO("Monitor thread is exiting");

    return NULL;
}
