#ifndef __CONFIG_H__
#define __CONFIG_H__

// 全局配置结构体
typedef struct {
    char cloud_ip[64];
    int  cloud_port;
    char device_id[64];
    char api_key[256];
    char log_path[256];
    
    // [新增] UDP 组播配置
    char multicast_ip[32];
    int  multicast_port;
    char local_ip[32];
} GatewayConfig;

extern GatewayConfig g_config;

int load_config(const char *filepath);

#endif // __CONFIG_H__
