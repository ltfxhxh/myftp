#include "thread_pool.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

static void *thread_do_work(void *pool);

// 创建线程池，初始化线程池结构
ThreadPool *thread_pool_create(int min_threads, int max_threads, int queue_size) {
    ThreadPool *pool;
    int i;

    if (min_threads <= 0 || max_threads <= 0 || queue_size <= 0 || min_threads > max_threads) {
        fprintf(stderr, "Error: Invalid arguments\n");
        return NULL;
    }

    // 分配线程池结构体内存
    pool = (ThreadPool *)malloc(sizeof(ThreadPool));
    if (pool == NULL) {
        perror("Failed to malloc for ThreadPool");
        return NULL;
    }

    // Initialize
    pool->thread_count = min_threads;
    pool->queue_size = queue_size;
    pool->head = pool->tail = pool->count = 0;
    pool->shutdown = pool->started = 0;
    pool->max_threads = max_threads;
    pool->min_threads = min_threads;
    pool->active_threads = 0;

    // Allocate thread and task queue
    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * max_threads);
    pool->task_queue = (ThreadPoolTask *)malloc(sizeof(ThreadPoolTask) * queue_size);

    if (pool->threads == NULL || pool->task_queue == NULL) {
        perror("Failed to malloc for threads or task_queue");
        thread_pool_free(pool);
        return NULL;
    }

    // Initialize mutex and conditional variable first
    if (pthread_mutex_init(&(pool->lock), NULL) != 0 ||
        pthread_cond_init(&(pool->notify), NULL) != 0) {
        perror("Failed to init mutex or cond");
        thread_pool_free(pool);
        return NULL;
    }

    // Start worker threads
    for (i = 0; i < min_threads; i++) {
        if (pthread_create(&(pool->threads[i]), NULL, thread_do_work, (void *)pool) != 0) {
            thread_pool_destroy(pool, 0);
            return NULL;
        }
        pool->started++;
    }

    return pool;
}

// Add a new task to the pool
int thread_pool_add(ThreadPool *pool, void (*function)(void *), void *arg) {
    int err = 0, next;

    if (pool == NULL || function == NULL) {
        return -1;
    }

    if (pthread_mutex_lock(&(pool->lock)) != 0) {
        return -1;
    }

    next = (pool->tail + 1) % pool->queue_size;

    do {
        // Calculate next position in queue
        if (pool->count == pool->queue_size) {
            err = -1;
            break;
        }

        if (pool->shutdown) {
            err = -1;
            break;
        }

        // Add task to queue
        pool->task_queue[pool->tail].function = function;
        pool->task_queue[pool->tail].arg = arg;
        pool->tail = next;
        pool->count += 1;

        // pthread_cond_broadcast
        if (pthread_cond_signal(&(pool->notify)) != 0) {
            err = -1;
            break;
        }
    } while (0);

    if (pthread_mutex_unlock(&pool->lock) != 0) {
        err = -1;
    }

    return err;
}

// Destroy the pool
int thread_pool_destroy(ThreadPool *pool, int grace) {
    int i, err = 0;

    if (pool == NULL) {
        return -1;
    }

    if (pthread_mutex_lock(&(pool->lock)) != 0) {
        return -1;
    }

    do {
        if (pool->shutdown) {
            err = -1;
            break;
        }

        pool->shutdown = (grace) ? 1 : 2;

        // Wake up all worker threads
        if (pthread_cond_broadcast(&(pool->notify)) != 0 || pthread_mutex_unlock(&(pool->lock)) != 0) {
            err = -1;
            break;
        }

        // Join all worker threads
        for (i = 0; i < pool->thread_count; i++) {
            if (pthread_join(pool->threads[i], NULL) != 0) {
                err = -1;
            }
        }
    } while (0);

    // Only if everything went well do we deallocate the pool
    if (!err) {
        thread_pool_free(pool);
    }
    return err;
}

// Free the pool resources
void thread_pool_free(ThreadPool *pool) {
    if (pool == NULL || pool->started > 0) {
        return;
    }

    // Did we manage to allocate?
    if (pool->threads) {
        free(pool->threads);
        free(pool->task_queue);

        // Because we allocate pool->threads after initializing the mutex and condition variable,
        // we're sure they're initialized at this point. So we can safely destroy them.
        pthread_mutex_lock(&(pool->lock));
        pthread_mutex_destroy(&(pool->lock));
        pthread_cond_destroy(&(pool->notify));
    }
    free(pool);
}

/*
 * 动态修改线程池
 * */
int thread_pool_resize(ThreadPool *pool, int new_size) {
    if (new_size < pool->min_threads || new_size > pool->max_threads) {
        fprintf(stderr, "Error: new_size out of range\n");
        return -1;
    }

    pthread_mutex_lock(&(pool->lock));
    int old_size = pool->thread_count;
    pool->thread_count = new_size;

    if (new_size > old_size) {
        // 增加线程
        for (int i = old_size; i < new_size; ++i) {
            if (pthread_create(&(pool->threads[i]), NULL, thread_do_work, (void *)pool) != 0) {
                pool->thread_count = i;
                break;
            }
            pool->started++;
        }
    } else if (new_size < old_size) {
        // 减少线程
        for (int i = new_size; i < old_size; ++i) {
            pool->threads[i] = 0;
        }
    }

    pthread_mutex_unlock(&(pool->lock));
    return 0;
}

// Thread working function
static void *thread_do_work(void *pool) {
    ThreadPool *p = (ThreadPool *)pool;
    ThreadPoolTask task;

    while (1) {
        pthread_mutex_lock(&(p->lock));

        // Wait while there are no tasks in the queue
        while ((p->count == 0) && (!p->shutdown)) {
            pthread_cond_wait(&(p->notify), &(p->lock));
        }

        if ((p->shutdown == 1) && (p->count == 0)) {
            break;
        }

        // Grab our task
        task.function = p->task_queue[p->head].function;
        task.arg = p->task_queue[p->head].arg;
        p->head = (p->head + 1) % p->queue_size;
        p->count--;

        // Unlock to allow other threads to operate on the queue
        pthread_mutex_unlock(&(p->lock));

        // Execute the task
        (*(task.function))(task.arg);
    }

    p->started--;

    pthread_mutex_unlock(&(p->lock));
    pthread_exit(NULL);
    return (NULL);
}

