#ifndef __GATEWAY_H__
#define __GATEWAY_H__

#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>

#define MAX_QUEUE_SIZE 1024

// 传感器数据结构 (网关内部与IPC通信通用)
typedef struct {
    uint32_t sensor_id;
    double   sensor_data;
} IPCData;

// --- 全局环形队列声明 (实现在 main.c 中) ---
extern IPCData data_queue[MAX_QUEUE_SIZE];
extern int queue_head;
extern int queue_tail;
extern pthread_mutex_t queue_mutex;
extern sem_t data_sem;

// 消费者线程入口
void* cloud_client_thread(void* arg);

#endif // __GATEWAY_H__
