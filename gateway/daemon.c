#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "daemon.h"

volatile sig_atomic_t g_running = 1;

// 信号处理回调函数
static void signal_handler(int sig) {
    switch (sig) {
        case SIGTERM:
        case SIGINT:
            // 在完整的框架中，收到退出信号后会将 g_running 设为 0，
            // 使得主循环(poll)安全退出，从而释放资源。
            g_running = 0; 
            printf("\n[系统] 收到退出信号(Ctrl+C)，正在安全退出...\n");
            
            // [新增] 临时强制退出，解决当前多线程阻塞无法结束的问题
            // 后续引入 poll 后，会由主循环检测 g_running 来平滑退出
            exit(0); 
            break;
        case SIGHUP:
            // 预留热加载配置的功能
            break;
    }
}

void setup_signals(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    
    // 捕获终止和中断信号
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    // 捕获挂起信号（常用来通知重新读取配置文件）
    sigaction(SIGHUP, &sa, NULL);
    
    // 忽略管道断开信号，防止向已关闭的socket写数据时程序崩溃
    signal(SIGPIPE, SIG_IGN); 
}

void daemonize(const char *pid_file) {
    pid_t pid;

    // 1. 第一次 fork：结束父进程，让子进程在后台执行
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS); 

    // 2. 脱离控制终端，创建一个新的会话
    if (setsid() < 0) exit(EXIT_FAILURE);

    // 3. 忽略 SIGCHLD 信号
    signal(SIGCHLD, SIG_IGN);

    // 4. 第二次 fork：确保进程不是会话首进程，防止再次获取控制终端
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    // 5. 设置文件权限掩码
    umask(0);

    // 注意：标准守护进程应该 chdir("/") 改变工作目录到根目录，防止占用可卸载的文件系统。
    // 但是这里为了方便读取当前目录的配置文件和日志，我们暂且留在当前目录运行。
    // chdir("/"); 

    // 6. 重定向标准输入、输出、错误到 /dev/null
    int fd = open("/dev/null", O_RDWR);
    if (fd != -1) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > 2) close(fd);
    }

    // 7. 生成 PID 文件并加锁防多开
    int pid_fd = open(pid_file, O_RDWR | O_CREAT, 0640);
    if (pid_fd < 0) exit(EXIT_FAILURE);
    
    // 尝试加写锁
    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    if (fcntl(pid_fd, F_SETLK, &fl) < 0) {
        // 加锁失败，说明已经有实例在运行
        exit(EXIT_FAILURE); 
    }
    
    // 将当前进程的 PID 写入文件
    char str[32];
    sprintf(str, "%d\n", getpid());
    
    // 增加对 write 返回值的检查，消除编译警告
    if (write(pid_fd, str, strlen(str)) < 0) {
        exit(EXIT_FAILURE); // 如果 PID 写入失败，视为启动失败退出
    }
    
    // 注意：不关闭 pid_fd，让进程一直持有该文件锁直到退出
}
