#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include <pthread.h>

// 任务结构体
typedef struct {
    void (*function)(void *); // 任务的回调执行函数
    void *argument;           // 传递给执行函数的参数
} threadpool_task_t;

// 线程池管理器结构体 (对外隐藏具体实现细节)
typedef struct threadpool_t threadpool_t;

/******************************
* 名    称 : threadpool_create()
* 功    能 : 创建并初始化一个固定大小的线程池
* 入口参数 : thread_count - 线程池中的工作线程数量
* queue_size   - 任务队列的最大容量
* 返回参数 : 成功返回线程池指针，失败返回 NULL
******************************/
threadpool_t *threadpool_create(int thread_count, int queue_size);

/******************************
* 名    称 : threadpool_add()
* 功    能 : 向线程池的任务队列中添加一个新任务
* 入口参数 : pool - 线程池指针
* function - 任务执行函数
* argument - 传递给任务函数的参数
* 返回参数 : 0 代表成功，-1 代表失败或队列已满
******************************/
int threadpool_add(threadpool_t *pool, void (*function)(void *), void *argument);

/******************************
* 名    称 : threadpool_destroy()
* 功    能 : 安全销毁线程池，等待所有任务执行完毕后回收资源
******************************/
int threadpool_destroy(threadpool_t *pool);

#endif // __THREAD_POOL_H__
