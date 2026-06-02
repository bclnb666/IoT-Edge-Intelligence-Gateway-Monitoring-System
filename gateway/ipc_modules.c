#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/time.h>
#include <string.h>
#include "ipc_modules.h"
#include "logger.h"

// System V IPC 的魔法键值 (必须全系统唯一)
#define SHM_KEY 0x1234
#define SEM_KEY 0x5678

static int shm_id = -1;
static int sem_id = -1;
static SharedSensorState *shared_mem = NULL;

// 信号量操作联合体 (Linux标准规范)
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

// 辅助函数：获取当前时间戳(毫秒)
static long long get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

// 信号量上锁 (P操作)
static void sem_lock() {
    if (sem_id < 0) return;
    struct sembuf sb = {0, -1, SEM_UNDO}; // 对第0个信号量减1
    semop(sem_id, &sb, 1);
}

// 信号量解锁 (V操作)
static void sem_unlock() {
    if (sem_id < 0) return;
    struct sembuf sb = {0, 1, SEM_UNDO};  // 对第0个信号量加1
    semop(sem_id, &sb, 1);
}

int init_system_v_ipc(void) {
    // 1. 创建共享内存 (大小为容纳 MAX_SENSORS 个状态结构体)
    // IPC_CREAT | 0666 表示如果不存在则创建，权限为读写
    shm_id = shmget(SHM_KEY, sizeof(SharedSensorState) * MAX_SENSORS, IPC_CREAT | 0666);
    if (shm_id < 0) {
        LOG_ERR("[SystemV] 共享内存创建失败");
        return -1;
    }

    // 2. 将这块物理内存映射到当前进程的虚拟地址空间
    shared_mem = (SharedSensorState *)shmat(shm_id, NULL, 0);
    if (shared_mem == (void *)-1) {
        LOG_ERR("[SystemV] 共享内存映射失败");
        return -1;
    }
    
    // 初始化清空共享内存
    memset(shared_mem, 0, sizeof(SharedSensorState) * MAX_SENSORS);

    // 3. 创建信号量集 (包含 1 个信号量，专门用来当做互斥锁)
    sem_id = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if (sem_id < 0) {
        LOG_ERR("[SystemV] 信号量创建失败");
        return -1;
    }

    // 4. 将信号量的初始值设置为 1 (代表现在可用，没人占用)
    union semun su;
    su.val = 1;
    if (semctl(sem_id, 0, SETVAL, su) < 0) {
        LOG_ERR("[SystemV] 信号量初始化失败");
        return -1;
    }

    LOG_INFO("[SystemV] 共享内存与信号量初始化成功! (支持缓存 %d 个节点实时状态)", MAX_SENSORS);
    return 0;
}

void update_shared_sensor_data(uint32_t sensor_id, double data) {
    if (shared_mem == NULL || shared_mem == (void *)-1) return;

    // 上锁，防止写到一半时其他进程读到错乱的数据
    sem_lock(); 

    int target_idx = -1;
    int empty_idx = -1;

    // 遍历共享内存，寻找对应的传感器ID，或者找一个空位
    for (int i = 0; i < MAX_SENSORS; i++) {
        if (shared_mem[i].sensor_id == sensor_id) {
            target_idx = i;
            break;
        }
        if (shared_mem[i].sensor_id == 0 && empty_idx == -1) {
            empty_idx = i; // 记录遇见的第一个空槽位
        }
    }

    // 决定写入位置：如果有老记录就更新，没有老记录就找空位插入
    int write_idx = (target_idx != -1) ? target_idx : empty_idx;
    
    if (write_idx != -1) {
        shared_mem[write_idx].sensor_id = sensor_id;
        shared_mem[write_idx].latest_data = data;
        shared_mem[write_idx].update_time = get_time_ms();
    } else {
        LOG_WARN("[SystemV] 共享内存已满，无法缓存新传感器节点状态!");
    }

    // 写完数据，解锁放行
    sem_unlock(); 
}

double read_shared_sensor_data(uint32_t sensor_id) {
    if (shared_mem == NULL || shared_mem == (void *)-1) return -999.0;

    // 上锁保护读过程
    sem_lock(); 
    
    double val = -999.0; // 默认无效值
    for (int i = 0; i < MAX_SENSORS; i++) {
        if (shared_mem[i].sensor_id == sensor_id) {
            val = shared_mem[i].latest_data;
            break;
        }
    }

    sem_unlock(); 
    return val;
}

void destroy_system_v_ipc(void) {
    if (shared_mem != (void *)-1 && shared_mem != NULL) {
        shmdt(shared_mem); // 解除映射
    }
    if (shm_id >= 0) {
        shmctl(shm_id, IPC_RMID, NULL); // 通知内核删除共享内存
    }
    if (sem_id >= 0) {
        semctl(sem_id, 0, IPC_RMID); // 通知内核删除信号量
    }
    LOG_INFO("[SystemV] 共享内存与信号量资源已安全销毁");
}
