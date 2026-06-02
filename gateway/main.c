#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <unistd.h>
#include "gateway.h"
#include "logger.h"
#include "password_auth.h" 
#include "config.h" 
#include "daemon.h" 
#include "thread_pool.h" 
#include "udp_server.h" 
#include "ipc_server.h"
#include "io_multiplexing.h"
#include "token_bucket.h" 
#include "alarm_scheduler.h" 
#include "ipc_modules.h" 
#include "process_pool.h" // [新增] 引入进程池模块

// ================= 全局资源定义 =================
IPCData data_queue[MAX_QUEUE_SIZE];
int queue_head = 0;
int queue_tail = 0;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t data_sem;

threadpool_t *g_threadpool = NULL;

// [修改] 让定时任务触发向进程池投递指令
void heartbeat_task(void *arg) {
    LOG_INFO("[心跳任务] 滴答！当前网关正在运行，准备通过 FIFO 派发任务给进程池...");
    
    // 主进程通过命名管道 (FIFO) 派发异步任务
    process_pool_submit_task("CHECK_SENSOR_STATUS");
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("用法: %s <配置文件路径> [-d]\n", argv[0]);
        return -1;
    }

    if (load_config(argv[1]) < 0) {
        printf("加载配置文件失败！\n");
        return -1;
    }

    if (shadow_authenticate() == 0) return -1; 

    if (argc >= 3 && strcmp(argv[2], "-d") == 0) {
        printf("正在转入后台守护进程模式运行...\n");
        daemonize("/tmp/iot_gateway.pid");
    }

    setup_signals();
    init_logger();
    sem_init(&data_sem, 0, 0);
    
    LOG_INFO("==== 物联网网关 (Reactor 模型) 启动 ====");

    // [新增] 初始化 System V 共享内存与信号量
    if (init_system_v_ipc() < 0) {
        return -1;
    }

    // [新增] 孵化进程池 (启动 2 个物理隔离的沙箱子进程)
    process_pool_init(2);

    token_bucket_init(100, 50);
    alarm_scheduler_init();
    alarm_scheduler_add_task(5, heartbeat_task, NULL);

    g_threadpool = threadpool_create(4, 256);
    if (g_threadpool == NULL) return -1;

    // 1. 初始化非阻塞的 Socket 数据源
    int udp_sock = init_udp_server();
    int ipc_sock = init_ipc_server();

    // 2. 启动单实例消费者 (未来可交给进程池处理)
    pthread_t cloud_tid;
    pthread_create(&cloud_tid, NULL, cloud_client_thread, NULL);

    // 3. 启动大心脏：进入 poll 循环阻塞监听 (程序将停留在此处)
    start_event_loop(udp_sock, ipc_sock);

    // 4. ======== 以下是安全退出流程 ========
    pthread_cancel(cloud_tid); 
    pthread_join(cloud_tid, NULL);

    alarm_scheduler_destroy(); 
    process_pool_destroy(); // [新增] 销毁进程池与命名管道
    destroy_system_v_ipc(); 
    if (g_threadpool) threadpool_destroy(g_threadpool);
    if (udp_sock >= 0) close(udp_sock);
    if (ipc_sock >= 0) close(ipc_sock);
    unlink("/tmp/gateway_ipc.sock");

    LOG_INFO("网关进程资源释放完毕，完全退出。");
    return 0;
}
