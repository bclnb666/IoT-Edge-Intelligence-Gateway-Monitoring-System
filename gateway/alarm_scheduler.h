#ifndef __ALARM_SCHEDULER_H__
#define __ALARM_SCHEDULER_H__

// 任务回调函数指针类型
typedef void (*alarm_task_func)(void *arg);

/******************************
* 名    称 : alarm_scheduler_init()
* 功    能 : 初始化闹钟调度器（基于 setitimer 和 SIGALRM）
******************************/
void alarm_scheduler_init(void);

/******************************
* 名    称 : alarm_scheduler_add_task()
* 功    能 : 注册一个周期性定时任务
* 入口参数 : interval_sec - 执行周期（秒）
* func         - 任务回调函数
* arg          - 传递给回调函数的参数
* 返回参数 : 任务槽ID (>=0 代表成功，-1 代表失败)
******************************/
int alarm_scheduler_add_task(int interval_sec, alarm_task_func func, void *arg);

/******************************
* 名    称 : alarm_scheduler_destroy()
* 功    能 : 销毁调度器，关闭系统闹钟
******************************/
void alarm_scheduler_destroy(void);

#endif // __ALARM_SCHEDULER_H__
