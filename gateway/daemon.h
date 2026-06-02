#ifndef __DAEMON_H__
#define __DAEMON_H__

#include <signal.h>

// 运行标志位，1 表示运行，0 表示停止（收到退出信号后改变）
extern volatile sig_atomic_t g_running;

/******************************
* 名    称 : daemonize()
* 功    能 : 标准 Linux 守护进程化流程，包含 PID 文件锁定防多开
* 入口参数 : pid_file - PID 文件的路径
******************************/
void daemonize(const char *pid_file);

/******************************
* 名    称 : setup_signals()
* 功    能 : 注册系统信号捕获 (SIGINT, SIGTERM, SIGHUP等)
******************************/
void setup_signals(void);

#endif // __DAEMON_H__
