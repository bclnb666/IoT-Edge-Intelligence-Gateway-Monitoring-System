#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <semaphore.h>
#include "gateway.h"
#include "base64.h"
#include "logger.h" 
#include "config.h" // [新增] 引入全局配置

void* cloud_client_thread(void* arg) {
    char base64_id[256];
    char temp_buf[512];
    
    // [修改] 使用全局配置中的 api_key
    sprintf(temp_buf, "%s:", g_config.api_key);
    base64_encode((unsigned char *)base64_id, (unsigned char *)temp_buf);

    LOG_INFO("[云端-客户] HTTP客户端线程已启动，准备就绪");

    while (1) {
        // [信号量机制]：阻塞等待。只要队列中没有数据(信号量为0)，就会一直休眠。
        // 一旦下位机发来数据，sem_post 触发，这里立刻放行并消耗 1 个信号。
        sem_wait(&data_sem);

        pthread_mutex_lock(&queue_mutex);
        
        IPCData batch[50]; // 我们每次最多批量提取 50 条数据进行打包上传
        int count = 0;
        
        // 既然已经被唤醒，我们先把触发唤醒的第一条数据拿出来
        batch[count++] = data_queue[queue_tail];
        queue_tail = (queue_tail + 1) % MAX_QUEUE_SIZE;

        // [批量提取优化]：看看队列里还有没有积压的其他数据，趁着加锁一次性带走
        while (count < 50 && sem_trywait(&data_sem) == 0) {
            batch[count++] = data_queue[queue_tail];
            queue_tail = (queue_tail + 1) % MAX_QUEUE_SIZE;
        }
        
        pthread_mutex_unlock(&queue_mutex);

        // 收集所有已取出的数据并组装 JSON payload
        char json_payload[4096]; 
        int json_len = sprintf(json_payload, "{\"datas\":[");

        for (int i = 0; i < count; i++) {
            if (i > 0) {
                json_len += sprintf(json_payload + json_len, ",");
            }
            json_len += sprintf(json_payload + json_len, "{\"id\":%u, \"data\":%f}", 
                                batch[i].sensor_id, batch[i].sensor_data);
        }
        json_len += sprintf(json_payload + json_len, "]}");

        // 组装完整的 HTTP POST 请求报文
        char http_request[8192];
        sprintf(http_request, 
            "POST /api/1.0/device/%s/datas HTTP/1.1\r\n"
            "Host: www.embsky.com\r\n"
            "Accept: */*\r\n"
            "Authorization: Basic %s\r\n"
            "Content-Length: %d\r\n"
            "Content-Type: application/json;charset=utf-8\r\n"
            "Connection: close\r\n\r\n"
            "%s", 
            g_config.device_id, base64_id, json_len, json_payload); // [修改] 使用 device_id

        // 建立 TCP 短连接并发送 HTTP 请求
        int sd = socket(AF_INET, SOCK_STREAM, 0);
        if (sd < 0) {
            LOG_ERR("[云端-客户] Socket创建失败");
            continue;
        }

        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(g_config.cloud_port); // [修改] 使用配置中的端口
        server_addr.sin_addr.s_addr = inet_addr(g_config.cloud_ip); // [修改] 使用配置中的 IP

        if (connect(sd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            LOG_ERR("[云端-客户] 连接云平台失败");
            close(sd);
            sleep(3); // 失败后歇一会再试
            continue;
        }

        if (write(sd, http_request, strlen(http_request)) < 0) {
            LOG_ERR("[云端-客户] HTTP请求发送失败");
        } else {
            // 读取云平台的 HTTP 响应
            char resp_buf[1024];
            int r = read(sd, resp_buf, sizeof(resp_buf) - 1);
            if (r > 0) {
                resp_buf[r] = '\0';
                if (strstr(resp_buf, "200 OK") != NULL) {
                    // 替换原先的 printf，使用日志宏
                    LOG_INFO("[云端-客户] 成功上传 %d 个传感器数据到云平台!", count);
                } else {
                    LOG_ERR("[云端-客户] 云平台返回异常响应:\n%s", resp_buf);
                }
            }
        }

        close(sd); 
        
        // [动态防限流]：如果当前队列已经空了，我们才休息 2 秒。
        // 如果队列里还有积压数据(说明下位机发得非常快)，就不休息，马上去处理下一批！
        if (queue_head == queue_tail) {
            sleep(2); 
        }
    }
    return NULL;
}
