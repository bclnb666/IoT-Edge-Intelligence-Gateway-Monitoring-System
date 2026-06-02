#ifndef __PROCESS_POOL_H__
#define __PROCESS_POOL_H__

/******************************
* 名    称 : process_pool_init()
* 功    能 : 创建命名管道并孵化一组隔离的子进程
* 入口参数 : count - 要创建的子进程数量
* 返回参数 : 成功返回0，失败返回-1
******************************/
int process_pool_init(int count);

/******************************
* 名    称 : process_pool_submit_task()
* 功    能 : 通过命名管道 (FIFO) 向子进程派发任务
* 入口参数 : task_cmd - 任务指令字符串
* 返回参数 : 成功返回0，失败返回-1
******************************/
int process_pool_submit_task(const char *task_cmd);

/******************************
* 名    称 : process_pool_destroy()
* 功    能 : 安全销毁进程池，清理命名管道并终止子进程
******************************/
void process_pool_destroy(void);

#endif // __PROCESS_POOL_H__
