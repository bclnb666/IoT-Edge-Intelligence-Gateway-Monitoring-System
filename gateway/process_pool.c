#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include "process_pool.h"
#include "logger.h"
#include "ipc_modules.h" 

// 命名管道存放路径
#define TASK_FIFO "/tmp/gateway_task_fifo"
#define MAX_CHILDREN 16

static pid_t child_pids[MAX_CHILDREN];
static int child_count = 0;
static int fifo_write_fd = -1;

// 子进程独享的执行沙箱
static void worker_process_logic(int id) {
    // 恢复被父进程修改过的信号默认处理，允许子进程正常被杀死
    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);

    // 打开 FIFO 读端 (阻塞模式等待任务)
    int fifo_read_fd = open(TASK_FIFO, O_RDONLY);
    if (fifo_read_fd < 0) exit(1);

    LOG_INFO("[进程池] 隔离沙箱子进程 %d (PID: %d) 启动成功，正在监听 FIFO...", id, getpid());

    char buf[256];
    while (1) {
        ssize_t n = read(fifo_read_fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            buf[strcspn(buf, "\r\n")] = 0; // 去除换行符
            
            // 收到主进程通过 FIFO 派发的指令
            if (strcmp(buf, "CHECK_SENSOR_STATUS") == 0) {
                // [高能联动]：跨进程访问 System V 共享内存“黑板”！
                double temp = read_shared_sensor_data(16790); // 16790是温度ID
                double hum = read_shared_sensor_data(16789);  // 16789是湿度ID
                
                LOG_INFO("[进程池-子进程%d] 收到指令! 查看共享内数据 -> 温度: %.1f, 湿度: %.1f", id, temp, hum);
            } else {
                LOG_INFO("[进程池-子进程%d] 执行未知的第三方插件任务: '%s'", id, buf);
            }
        } else if (n == 0) {
            // 写端全部关闭，子进程安全退出
            break;
        }
    }
    
    close(fifo_read_fd);
    exit(0); // 子进程结束
}

int process_pool_init(int count) {
    if (count > MAX_CHILDREN) count = MAX_CHILDREN;
    child_count = count;

    // 1. 创建命名管道 (FIFO)
    unlink(TASK_FIFO);
    if (mkfifo(TASK_FIFO, 0666) < 0) {
        LOG_ERR("[进程池] 命名管道 FIFO 创建失败");
        return -1;
    }

    // 2. 连续孵化子进程
    for (int i = 0; i < count; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            LOG_ERR("[进程池] fork 失败");
            return -1;
        } else if (pid == 0) {
            // 子进程被困在此处，永远不会返回 main
            worker_process_logic(i + 1); 
        } else {
            // 父进程记录子进程的 PID
            child_pids[i] = pid;
        }
    }

    // 3. 主进程以读写模式打开 FIFO 写端 (防阻塞)
    fifo_write_fd = open(TASK_FIFO, O_RDWR);
    if (fifo_write_fd < 0) {
        LOG_ERR("[进程池] 打开 FIFO 写端失败");
        return -1;
    }

    LOG_INFO("[进程池] 初始化完成! 成功孵化 %d 个子进程，开启 IPC 命名管道", count);
    
    // 【关于僵尸进程收割】：
    // 由于我们在 daemon.c 中已经执行过 signal(SIGCHLD, SIG_IGN);
    // Linux 内核会自动收割任何死掉的子进程，不会产生僵尸进程！(完美符合文档需求)
    
    return 0;
}

int process_pool_submit_task(const char *task_cmd) {
    if (fifo_write_fd < 0 || task_cmd == NULL) return -1;
    
    char buf[256];
    snprintf(buf, sizeof(buf), "%s\n", task_cmd);
    
    // 向 FIFO 写入任务，多子进程抢占式读取，Linux 保证小数据包写入的原子性
    if (write(fifo_write_fd, buf, strlen(buf)) < 0) {
        return -1;
    }
    return 0;
}

void process_pool_destroy(void) {
    if (fifo_write_fd >= 0) close(fifo_write_fd);
    
    // 发送信号强制终止所有子进程
    for (int i = 0; i < child_count; i++) {
        if (child_pids[i] > 0) {
            kill(child_pids[i], SIGKILL);
        }
    }
    unlink(TASK_FIFO); // 销毁管道文件
    LOG_INFO("[进程池] 命名管道已断开，子进程已强制回收");
}
