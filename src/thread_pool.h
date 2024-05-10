#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>

// 线程池任务结构体
typedef struct ThreadPoolTask {
    void (*function)(void *);  // 函数指针，指向任务的具体处理函数
    void *arg;                 // 函数参数
} ThreadPoolTask;

// 线程池结构体
typedef struct ThreadPool {
    pthread_mutex_t lock;           // 互斥锁，用于同步访问
    pthread_cond_t notify;          // 条件变量，用于通知有任务到来
    pthread_t *threads;             // 线程数组
    ThreadPoolTask *task_queue;     // 任务队列
    int thread_count;               // 线程数量
    int queue_size;                 // 任务队列的大小
    int head;                       // 任务队列的头部索引
    int tail;                       // 任务队列的尾部索引
    int count;                      // 任务队列中当前的任务数量
    int shutdown;                   // 标记线程池是否关闭
    int started;                    // 已启动的线程数量
} ThreadPool;

// 函数声明
ThreadPool *thread_pool_create(int thread_count, int queue_size);
int thread_pool_add(ThreadPool *pool, void (*function)(void *), void *arg);
int thread_pool_destroy(ThreadPool *pool, int grace);
void thread_pool_free(ThreadPool *pool);

#endif // THREAD_POOL_H

