#ifndef __IPC_MODULES_H__
#define __IPC_MODULES_H__

#include <stdint.h>

// 支持缓存的最大传感器节点数量
#define MAX_SENSORS 100

// 共享内存中保存的传感器状态结构体
typedef struct {
    uint32_t sensor_id;    // 传感器ID
    double   latest_data;  // 最新一次上报的数值
    long long update_time; // 最新一次上报的时间戳(毫秒)
} SharedSensorState;

/******************************
* 名    称 : init_system_v_ipc()
* 功    能 : 初始化共享内存与保护它的信号量
* 返回参数 : 成功返回0，失败返回-1
******************************/
int init_system_v_ipc(void);

/******************************
* 名    称 : update_shared_sensor_data()
* 功    能 : 刷新指定传感器的最新状态到共享内存中
* 入口参数 : sensor_id - 传感器ID, data - 最新数值
******************************/
void update_shared_sensor_data(uint32_t sensor_id, double data);

/******************************
* 名    称 : read_shared_sensor_data()
* 功    能 : 从共享内存中安全读取指定传感器的最新数据
* 入口参数 : sensor_id - 传感器ID
* 返回参数 : 传感器数值，如果未找到返回 -999.0
******************************/
double read_shared_sensor_data(uint32_t sensor_id);

/******************************
* 名    称 : destroy_system_v_ipc()
* 功    能 : 销毁 System V 共享内存与信号量资源
******************************/
void destroy_system_v_ipc(void);

#endif // __IPC_MODULES_H__
