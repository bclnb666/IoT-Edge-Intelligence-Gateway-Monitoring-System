#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <errno.h>
#include "gateway.h"
#include "logger.h"
#include "udp_server.h"
#include "ipc_server.h"
#include "io_multiplexing.h"
#include "daemon.h"

#define MAX_POLL_FDS 1024

void start_event_loop(int udp_sock, int ipc_sock) {
    struct pollfd fds[MAX_POLL_FDS];
    int nfds = 0;

    // 初始化所有槽位
    for (int i = 0; i < MAX_POLL_FDS; i++) {
        fds[i].fd = -1;
        fds[i].events = 0;
        fds[i].revents = 0;
    }

    // 1. 注册 UDP 监听套接字
    if (udp_sock >= 0) {
        fds[nfds].fd = udp_sock;
        fds[nfds].events = POLLIN;
        nfds++;
    }

    // 2. 注册 IPC 监听套接字
    if (ipc_sock >= 0) {
        fds[nfds].fd = ipc_sock;
        fds[nfds].events = POLLIN;
        nfds++;
    }

    LOG_INFO("==== 网关异步事件驱动引擎 (poll) 已全面接管 ====");

    while (g_running) {
        int ready = poll(fds, nfds, -1);
        
        if (ready < 0) {
            // [新增/修复] 捕获被信号 (如 SIGALRM 闹钟) 中断的正常现象
            if (errno == EINTR) {
                continue; // 忽略中断，重新进入 poll 阻塞监听
            }
            if (g_running == 0) break; 
            LOG_ERR("[主循环] poll 系统调用出错");
            break;
        }

        // 动态遍历所有发生了事件的 Socket
        for (int i = 0; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                
                // 【事件 1】：UDP 组播收到了下位机数据
                if (fds[i].fd == udp_sock) {
                    handle_udp_read(udp_sock);
                }
                
                // 【事件 2】：IPC 发现有新的本地客户端（mock_device）请求连接
                else if (fds[i].fd == ipc_sock) {
                    int client_fd = accept(ipc_sock, NULL, NULL);
                    if (client_fd >= 0) {
                        if (nfds < MAX_POLL_FDS) {
                            // 将新来的客户端也加入 poll 的监听大名单
                            fds[nfds].fd = client_fd;
                            fds[nfds].events = POLLIN;
                            nfds++;
                            LOG_INFO("[主循环] 接受新的本地 IPC 客户端 (fd=%d)", client_fd);
                        } else {
                            close(client_fd);
                            LOG_WARN("[主循环] 监听列表已满，拒绝新 IPC 客户端");
                        }
                    }
                }
                
                // 【事件 3】：某一个已连接的 IPC 客户端发来了真实数据
                else {
                    IPCData data;
                    int bytes_read = read(fds[i].fd, &data, sizeof(data));
                    
                    if (bytes_read > 0) {
                        // 解析成功，推入环形队列
                        pthread_mutex_lock(&queue_mutex);
                        int next_head = (queue_head + 1) % MAX_QUEUE_SIZE;
                        if (next_head != queue_tail) {
                            data_queue[queue_head].sensor_id = data.sensor_id;
                            data_queue[queue_head].sensor_data = data.sensor_data;
                            queue_head = next_head;
                            pthread_mutex_unlock(&queue_mutex);
                            sem_post(&data_sem); // 唤醒消费者上传云端
                            LOG_INFO("[IPC-事件] 捕获数据: ID=%u, Value=%.2f", data.sensor_id, data.sensor_data);
                        } else {
                            pthread_mutex_unlock(&queue_mutex);
                        }
                    } else {
                        // bytes_read <= 0 代表客户端已断开或异常
                        LOG_INFO("[主循环] IPC 客户端断开 (fd=%d)，释放资源", fds[i].fd);
                        close(fds[i].fd);
                        
                        // 从监听数组中剔除：用数组最后一个元素填补这个空缺，保持连续性
                        fds[i] = fds[nfds - 1];
                        fds[nfds - 1].fd = -1;
                        nfds--;
                        i--; // 索引回退一步，防止漏检刚搬过来的元素
                    }
                }
            }
        }
    }
    
    LOG_INFO("[主循环] 收到退出信号，事件引擎安全停机.");
}
