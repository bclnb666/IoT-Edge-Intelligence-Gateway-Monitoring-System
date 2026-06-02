#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "thread_pool.h"
#include "logger.h"

// 线程池结构体完整定义
struct threadpool_t {
    pthread_mutex_t lock;           // 互斥锁，保护整个结构体
    pthread_cond_t  notify;         // 条件变量，任务队列有任务时唤醒工作线程
    pthread_t       *threads;       // 存放工作线程ID的数组
    threadpool_task_t *queue;       // 任务队列（环形数组）
    int thread_count;               // 线程数量
    int queue_size;                 // 任务队列最大容量
    int head;                       // 队列头索引（取任务用）
    int tail;                       // 队列尾索引（存任务用）
    int count;                      // 当前队列中的任务数
    int shutdown;                   // 线程池销毁标志（1代表正在销毁）
};

// 工作线程的内部执行函数
static void *threadpool_worker(void *threadpool) {
    threadpool_t *pool = (threadpool_t *)threadpool;
    threadpool_task_t task;

    while (1) {
        pthread_mutex_lock(&(pool->lock));

        // 如果队列为空且没有收到销毁信号，工作线程进入休眠等待
        while ((pool->count == 0) && (!pool->shutdown)) {
            pthread_cond_wait(&(pool->notify), &(pool->lock));
        }

        // 如果收到销毁信号，立刻释放锁并退出线程
        if (pool->shutdown) {
            pthread_mutex_unlock(&(pool->lock));
            pthread_exit(NULL);
        }

        // 从环形队列头部取出一个任务
        task.function = pool->queue[pool->head].function;
        task.argument = pool->queue[pool->head].argument;
        pool->head = (pool->head + 1) % pool->queue_size;
        pool->count -= 1;

        pthread_mutex_unlock(&(pool->lock));

        // 真正开始执行任务（此时不占有锁，支持任务高并发执行）
        (*(task.function))(task.argument);
    }
    return NULL;
}

threadpool_t *threadpool_create(int thread_count, int queue_size) {
    threadpool_t *pool;
    
    if (thread_count <= 0 || queue_size <= 0) return NULL;

    pool = (threadpool_t *)malloc(sizeof(threadpool_t));
    if (pool == NULL) return NULL;

    pool->thread_count = 0;
    pool->queue_size = queue_size;
    pool->head = pool->tail = pool->count = 0;
    pool->shutdown = 0;

    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_count);
    pool->queue = (threadpool_task_t *)malloc(sizeof(threadpool_task_t) * queue_size);

    if ((pthread_mutex_init(&(pool->lock), NULL) != 0) ||
        (pthread_cond_init(&(pool->notify), NULL) != 0) ||
        (pool->threads == NULL) || (pool->queue == NULL)) {
        // 初始化失败时的资源清理...
        if (pool) threadpool_destroy(pool);
        return NULL;
    }

    // 启动所有的工作线程
    for (int i = 0; i < thread_count; i++) {
        if (pthread_create(&(pool->threads[i]), NULL, threadpool_worker, (void*)pool) != 0) {
            threadpool_destroy(pool);
            return NULL;
        }
        pool->thread_count++;
    }

    LOG_INFO("[线程池] 成功初始化，工作线程数: %d, 任务队列容量: %d", thread_count, queue_size);
    return pool;
}

int threadpool_add(threadpool_t *pool, void (*function)(void *), void *argument) {
    int err = 0;
    if (pool == NULL || function == NULL) return -1;

    if (pthread_mutex_lock(&(pool->lock)) != 0) return -1;

    // 队列已满，添加失败
    if (pool->count == pool->queue_size) {
        err = -1;
    } else if (pool->shutdown) { // 正在销毁，拒绝接收新任务
        err = -1;
    } else {
        // 将新任务加入环形队列尾部
        pool->queue[pool->tail].function = function;
        pool->queue[pool->tail].argument = argument;
        pool->tail = (pool->tail + 1) % pool->queue_size;
        pool->count += 1;

        // 发送信号，唤醒一个空闲的工人线程来干活
        if (pthread_cond_signal(&(pool->notify)) != 0) {
            err = -1;
        }
    }

    pthread_mutex_unlock(&(pool->lock));
    return err;
}

int threadpool_destroy(threadpool_t *pool) {
    if (pool == NULL) return -1;

    if (pthread_mutex_lock(&(pool->lock)) != 0) return -1;
    if (pool->shutdown) {
        pthread_mutex_unlock(&(pool->lock));
        return -1;
    }

    // 设置销毁标志位
    pool->shutdown = 1;

    // 唤醒所有正在休眠的工人线程，让它们看到 shutdown 标志后集体自杀
    if ((pthread_cond_broadcast(&(pool->notify)) != 0) ||
        (pthread_mutex_unlock(&(pool->lock)) != 0)) {
        return -1;
    }

    // 回收所有线程的资源
    for (int i = 0; i < pool->thread_count; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    // 销毁锁和条件变量，释放内存
    pthread_mutex_destroy(&(pool->lock));
    pthread_cond_destroy(&(pool->notify));
    
    if (pool->threads) free(pool->threads);
    if (pool->queue) free(pool->queue);
    free(pool);

    LOG_INFO("[线程池] 资源已安全销毁");
    return 0;
}
