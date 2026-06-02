#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>
#include "alarm_scheduler.h"
#include "logger.h"
#include "thread_pool.h"

// 支持同时注册的最大定时任务数量
#define MAX_ALARM_TASKS 16

// 定时任务结构体
typedef struct {
    int in_use;             // 槽位是否被占用
    int interval;           // 任务触发周期 (秒)
    int ticks_left;         // 距离下次触发还剩多少秒
    alarm_task_func func;   // 任务回调函数
    void *arg;              // 参数
} alarm_task_t;

static alarm_task_t tasks[MAX_ALARM_TASKS];
extern threadpool_t *g_threadpool; // 引用外部的全局线程池

// 核心：系统闹钟信号 (SIGALRM) 中断处理函数
static void handle_sigalrm(int sig) {
    // 遍历所有已注册的任务
    for (int i = 0; i < MAX_ALARM_TASKS; i++) {
        if (tasks[i].in_use) {
            tasks[i].ticks_left--; // 时间流逝 1 秒
            
            // 如果倒计时归零，说明该触发任务了
            if (tasks[i].ticks_left <= 0) {
                // 重置倒计时，准备下一次周期触发
                tasks[i].ticks_left = tasks[i].interval;
                
                // 【高并发架构精髓】
                // 绝不在内核中断的信号处理函数里执行耗时的业务逻辑！
                // 而是将任务打包，潇洒地丢给后台的工人线程池去排队执行。
                if (g_threadpool) {
                    threadpool_add(g_threadpool, tasks[i].func, tasks[i].arg);
                }
            }
        }
    }
}

void alarm_scheduler_init(void) {
    memset(tasks, 0, sizeof(tasks));

    // 1. 注册 SIGALRM 信号的捕获函数
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigalrm;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // 保证 poll 即使被信号打断也能自动重启
    
    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        LOG_ERR("[调度器] 注册 SIGALRM 失败");
        return;
    }

    // 2. 配置 setitimer，让系统每隔 1 秒向进程发送一次 SIGALRM 信号
    struct itimerval timer;
    timer.it_value.tv_sec = 1;      // 首次触发延迟 1 秒
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 1;   // 之后每隔 1 秒触发一次
    timer.it_interval.tv_usec = 0;

    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        LOG_ERR("[调度器] 启动 setitimer 系统闹钟失败");
        return;
    }

    LOG_INFO("[调度器] 闹钟调度器初始化完成 (内核信号驱动, 精度: 1秒)");
}

int alarm_scheduler_add_task(int interval_sec, alarm_task_func func, void *arg) {
    if (interval_sec <= 0 || func == NULL) return -1;
    
    // 短暂屏蔽 SIGALRM 信号，防止在添加任务的过程中被打断产生数据竞争
    sigset_t mask, old_mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGALRM);
    sigprocmask(SIG_BLOCK, &mask, &old_mask);

    int task_id = -1;
    for (int i = 0; i < MAX_ALARM_TASKS; i++) {
        if (!tasks[i].in_use) {
            tasks[i].interval = interval_sec;
            tasks[i].ticks_left = interval_sec;
            tasks[i].func = func;
            tasks[i].arg = arg;
            tasks[i].in_use = 1;
            task_id = i;
            break;
        }
    }
    
    // 恢复信号接收
    sigprocmask(SIG_SETMASK, &old_mask, NULL);
    
    if (task_id >= 0) {
        LOG_INFO("[调度器] 成功注册周期任务 (ID: %d, 触发周期: %d 秒)", task_id, interval_sec);
    } else {
        LOG_ERR("[调度器] 任务槽已满，无法注册新任务");
    }
    return task_id;
}

void alarm_scheduler_destroy(void) {
    // 写入全 0 关闭系统定时器
    struct itimerval timer = {0};
    setitimer(ITIMER_REAL, &timer, NULL);
    memset(tasks, 0, sizeof(tasks));
    LOG_INFO("[调度器] 资源已安全销毁");
}
