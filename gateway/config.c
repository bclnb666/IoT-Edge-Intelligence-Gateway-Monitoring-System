#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "config.h"

// 实例化全局配置变量
GatewayConfig g_config;

// 辅助函数：去除字符串首尾的空白字符和回车换行
static void trim(char *str) {
    if (!str) return;
    char *p1 = str;
    char *p2 = str + strlen(str) - 1;
    while (*p1 && isspace((unsigned char)*p1)) p1++;
    while (p2 > p1 && isspace((unsigned char)*p2)) *p2-- = '\0';
    memmove(str, p1, p2 - p1 + 2);
}

int load_config(const char *filepath) {
    FILE *fp = fopen(filepath, "r");
    if (!fp) return -1;

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        trim(line);
        if (line[0] == '\0' || line[0] == '#') continue;

        char *eq_pos = strchr(line, '=');
        if (!eq_pos) continue;

        *eq_pos = '\0'; 
        char *key = line;
        char *value = eq_pos + 1;
        
        trim(key);
        trim(value);

        if (strcmp(key, "CLOUD_IP") == 0) {
            strncpy(g_config.cloud_ip, value, sizeof(g_config.cloud_ip) - 1);
        } else if (strcmp(key, "CLOUD_PORT") == 0) {
            g_config.cloud_port = atoi(value);
        } else if (strcmp(key, "DEVICE_ID") == 0) {
            strncpy(g_config.device_id, value, sizeof(g_config.device_id) - 1);
        } else if (strcmp(key, "API_KEY") == 0) {
            strncpy(g_config.api_key, value, sizeof(g_config.api_key) - 1);
        } else if (strcmp(key, "LOG_PATH") == 0) {
            strncpy(g_config.log_path, value, sizeof(g_config.log_path) - 1);
        } else if (strcmp(key, "MULTICAST_IP") == 0) { // [新增] 解析组播IP
            strncpy(g_config.multicast_ip, value, sizeof(g_config.multicast_ip) - 1);
        } else if (strcmp(key, "MULTICAST_PORT") == 0) { // [新增] 解析组播端口
            g_config.multicast_port = atoi(value);
        } else if (strcmp(key, "LOCAL_IP") == 0) { // [新增] 解析本地网卡IP
            strncpy(g_config.local_ip, value, sizeof(g_config.local_ip) - 1);
        }
    }
    fclose(fp);
    return 0;
}
