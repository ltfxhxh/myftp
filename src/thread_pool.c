#include "thread_pool.h"
#include "logger.h"  // Include the logger header
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

static void *thread_do_work(void *pool);

// 创建线程池，初始化线程池结构
ThreadPool *thread_pool_create(int min_threads, int max_threads, int queue_size) {
    ThreadPool *pool;
    int i;

    LOG_INFO("Creating thread pool with MinThreads=%d, MaxThreads=%d, QueueSize=%d",
             min_threads, max_threads, queue_size);

    if (min_threads <= 0 || max_threads <= 0 || queue_size <= 0 || min_threads > max_threads) {
        LOG_ERROR("Invalid arguments for creating thread pool. MinThreads=%d, MaxThreads=%d, QueueSize=%d",
                  min_threads, max_threads, queue_size);
        return NULL;
    }

    // 分配线程池结构体内存
    pool = (ThreadPool *)malloc(sizeof(ThreadPool));
    if (pool == NULL) {
        LOG_ERROR("Failed to allocate memory for thread pool.");
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
        LOG_ERROR("Failed to allocate memory for threads or task queue.");
        thread_pool_free(pool);
        return NULL;
    }

    // Initialize mutex and conditional variable first
    if (pthread_mutex_init(&(pool->lock), NULL) != 0 ||
        pthread_cond_init(&(pool->notify), NULL) != 0) {
        LOG_ERROR("Failed to initialize mutex or condition variable.");
        thread_pool_free(pool);
        return NULL;
    }

    // Start worker threads
    for (i = 0; i < min_threads; i++) {
        if (pthread_create(&(pool->threads[i]), NULL, thread_do_work, (void *)pool) != 0) {
            LOG_ERROR("Failed to start worker thread %d.", i);
            thread_pool_destroy(pool, 0);
            return NULL;
        }
        pool->started++;
        LOG_DEBUG("Worker thread %d started.", i);
    }

    LOG_INFO("Thread pool created successfully.");
    return pool;
}

// Add a new task to the pool
int thread_pool_add(ThreadPool *pool, void (*function)(void *), void *arg) {
    int err = 0, next;

    if (pool == NULL || function == NULL) {
        LOG_ERROR("Invalid parameters to thread_pool_add.");
        return -1;
    }

    if (pthread_mutex_lock(&(pool->lock)) != 0) {
        LOG_ERROR("Failed to lock mutex in thread_pool_add.");
        return -1;
    }

    next = (pool->tail + 1) % pool->queue_size;

    do {
        // Calculate next position in queue
        if (pool->count == pool->queue_size) {
            err = -1;
            LOG_ERROR("Thread pool queue is full.");
            break;
        }

        if (pool->shutdown) {
            err = -1;
            LOG_ERROR("Thread pool is shutting down.");
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
            LOG_ERROR("Failed to signal condition variable in thread_pool_add.");
            break;
        }
        LOG_DEBUG("Task added to thread pool queue.");
    } while (0);

    if (pthread_mutex_unlock(&pool->lock) != 0) {
        err = -1;
        LOG_ERROR("Failed to unlock mutex in thread_pool_add.");
    }

    return err;
}

// Destroy the pool
int thread_pool_destroy(ThreadPool *pool, int grace) {
    int i, err = 0;

    if (pool == NULL) {
        LOG_ERROR("Attempt to destroy a NULL thread pool.");
        return -1;
    }

    if (pthread_mutex_lock(&(pool->lock)) != 0) {
        LOG_ERROR("Failed to lock mutex in thread_pool_destroy.");
        return -1;
    }

    do {
        if (pool->shutdown) {
            err = -1;
            LOG_ERROR("Attempt to destroy an already shutdown thread pool.");
            break;
        }

        pool->shutdown = (grace) ? 1 : 2;

        // Wake up all worker threads
        if (pthread_cond_broadcast(&(pool->notify)) != 0 || pthread_mutex_unlock(&(pool->lock)) != 0) {
            err = -1;
            LOG_ERROR("Failed to wake up all worker threads or unlock mutex in thread_pool_destroy.");
            break;
        }

        // Join all worker threads
        for (i = 0; i < pool->thread_count; i++) {
            if (pthread_join(pool->threads[i], NULL) != 0) {
                err = -1;
                LOG_ERROR("Failed to join worker thread %d in thread_pool_destroy.", i);
            }
        }
    } while (0);

    // Only if everything went well do we deallocate the pool
    if (!err) {
        thread_pool_free(pool);
        LOG_INFO("Thread pool destroyed successfully.");
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
    LOG_INFO("Thread pool resources freed.");
}

/*
 * 动态修改线程池
 * */
int thread_pool_resize(ThreadPool *pool, int new_size) {
    if (new_size < pool->min_threads || new_size > pool->max_threads) {
        LOG_ERROR("Attempt to resize thread pool out of bounds. NewSize=%d, MinThreads=%d, MaxThreads=%d",
                  new_size, pool->min_threads, pool->max_threads);
        return -1;
    }

    pthread_mutex_lock(&(pool->lock));
    int old_size = pool->thread_count;
    pool->thread_count = new_size;

    if (new_size > old_size) {
        // Increase the number of threads
        for (int i = old_size; i < new_size; ++i) {
            if (pthread_create(&(pool->threads[i]), NULL, thread_do_work, (void *)pool) != 0) {
                pool->thread_count = i;
                LOG_ERROR("Failed to increase the size of the thread pool. CurrentSize=%d, NewSize=%d",
                          i, new_size);
                break;
            }
            pool->started++;
            LOG_INFO("Increased thread pool size. New thread %d started.", i);
        }
    } else if (new_size < old_size) {
        // Decrease the number of threads
        for (int i = new_size; i < old_size; ++i) {
            if (pthread_cancel(pool->threads[i]) == 0) {
                LOG_INFO("Cancelled thread %d during pool resize.", i);
            }
        }
        LOG_INFO("Reduced thread pool size from %d to %d.", old_size, new_size);
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
        LOG_DEBUG("Executed a task in the thread pool.");
    }

    p->started--;

    pthread_mutex_unlock(&(p->lock));
    pthread_exit(NULL);
    return (NULL);
}
