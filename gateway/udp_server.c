#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include "gateway.h"
#include "config.h"
#include "logger.h"
#include "udp_server.h"
#include "token_bucket.h"
#include "ipc_modules.h" 

#define BUFFER_SIZE 1024

int init_udp_server(void) {
    int sock;
    struct sockaddr_in local_addr;
    struct ip_mreq mreq;

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        LOG_ERR("[UDP-服务] Socket 创建失败");
        return -1;
    }

    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        LOG_ERR("[UDP-服务] 设置 SO_REUSEADDR 失败");
        close(sock);
        return -1;
    }

    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(g_config.multicast_port);
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        LOG_ERR("[UDP-服务] 绑定端口 %d 失败", g_config.multicast_port);
        close(sock);
        return -1;
    }

    mreq.imr_multiaddr.s_addr = inet_addr(g_config.multicast_ip);
    
    if (strlen(g_config.local_ip) == 0) {
        strcpy(g_config.local_ip, "0.0.0.0");
    }

    if (strcmp(g_config.local_ip, "0.0.0.0") == 0) {
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    } else {
        mreq.imr_interface.s_addr = inet_addr(g_config.local_ip);
    }
    
    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        LOG_ERR("[UDP-服务] 加入组播组失败: %m (IP: %s)", g_config.local_ip);
        close(sock);
        return -1;
    }

    LOG_INFO("[UDP-服务] 非阻塞组播服务初始化成功! 监听: %s:%d (fd=%d)", 
             g_config.multicast_ip, g_config.multicast_port, sock);
             
    return sock; 
}

void handle_udp_read(int sock) {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in sender_addr;
    socklen_t sender_len = sizeof(sender_addr);
    
    int n = recvfrom(sock, buffer, BUFFER_SIZE - 1, 0, 
                     (struct sockaddr *)&sender_addr, &sender_len);
                     
    if (n <= 0) return;
    buffer[n] = '\0'; 

    if (token_bucket_consume(1) == 0) {
        LOG_WARN("[限流器] UDP rate limit exceeded, dropping packet from %s", inet_ntoa(sender_addr.sin_addr));
        return;
    }

    uint32_t sensor_id = 0;
    double sensor_data = 0.0;
    
    // 1. 尝试解析合并的 JSON 格式
    char *temp_str = strstr(buffer, "\"temp\":");
    char *hum_str = strstr(buffer, "\"hum\":");
    
    if (temp_str != NULL && hum_str != NULL) {
        double temp_val = 0.0, hum_val = 0.0;
        
        sscanf(temp_str + 7, "%lf", &temp_val);
        sscanf(hum_str + 6, "%lf", &hum_val);
        
        // --- 提取成功，将【温度】推入队列 ---
        pthread_mutex_lock(&queue_mutex);
        int next_head = (queue_head + 1) % MAX_QUEUE_SIZE;
        if (next_head != queue_tail) {
            data_queue[queue_head].sensor_id = 16790;
            data_queue[queue_head].sensor_data = temp_val;
            queue_head = next_head;
            pthread_mutex_unlock(&queue_mutex);
            sem_post(&data_sem); 
            
            update_shared_sensor_data(16790, temp_val);
            LOG_INFO("[UDP-事件] 触发解析成功(%s) -> 温度: %.1f", inet_ntoa(sender_addr.sin_addr), temp_val);
        } else {
            pthread_mutex_unlock(&queue_mutex);
        }

        // --- 提取成功，将【湿度】推入队列 ---
        pthread_mutex_lock(&queue_mutex);
        next_head = (queue_head + 1) % MAX_QUEUE_SIZE;
        if (next_head != queue_tail) {
            data_queue[queue_head].sensor_id = 16789;
            data_queue[queue_head].sensor_data = hum_val;
            queue_head = next_head;
            pthread_mutex_unlock(&queue_mutex);
            sem_post(&data_sem); 
            
            update_shared_sensor_data(16789, hum_val);
            LOG_INFO("[UDP-事件] 触发解析成功(%s) -> 湿度: %.1f", inet_ntoa(sender_addr.sin_addr), hum_val);
        } else {
            pthread_mutex_unlock(&queue_mutex);
        }
    }
    // 2. 兼容解析旧的单条数据格式 (之前漏改了这里！)
    else if (sscanf(buffer, "%u,%lf", &sensor_id, &sensor_data) == 2 ||
        sscanf(buffer, "%u %lf", &sensor_id, &sensor_data) == 2 ||
        sscanf(buffer, "{\"id\":%u, \"data\":%lf}", &sensor_id, &sensor_data) == 2) 
    {
        pthread_mutex_lock(&queue_mutex);
        int next_head = (queue_head + 1) % MAX_QUEUE_SIZE;
        if (next_head != queue_tail) {
            data_queue[queue_head].sensor_id = sensor_id;
            data_queue[queue_head].sensor_data = sensor_data;
            queue_head = next_head;
            pthread_mutex_unlock(&queue_mutex);
            sem_post(&data_sem); 
            
            // [修复]：增加这行！将单条数据也同步更新到 System V 共享内存“黑板”上！
            update_shared_sensor_data(sensor_id, sensor_data);
            
            LOG_INFO("[UDP-事件] 接管下位机(%s)单条数据: ID=%u, Value=%.2f", 
                     inet_ntoa(sender_addr.sin_addr), sensor_id, sensor_data);
        } else {
            pthread_mutex_unlock(&queue_mutex);
            LOG_WARN("[UDP-事件] 队列已满，丢弃下位机数据");
        }
    } else {
        LOG_WARN("[UDP-事件] 收到未知格式内容来自 %s: %s", 
                 inet_ntoa(sender_addr.sin_addr), buffer);
    }
}
