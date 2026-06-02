#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include "token_bucket.h"
#include "logger.h"

static int tb_capacity;             // 桶的容量
static int tb_rate;                 // 放入速率 (个/秒)
static double tb_tokens;            // 当前桶中的令牌数
static long long tb_last_time_ms;   // 上一次更新令牌的时间戳 (毫秒)
static pthread_mutex_t tb_mutex = PTHREAD_MUTEX_INITIALIZER;

// 获取当前系统时间的毫秒数
static long long get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void token_bucket_init(int capacity, int rate) {
    tb_capacity = capacity;
    tb_rate = rate;
    tb_tokens = capacity; // 初始状态桶是满的
    tb_last_time_ms = get_time_ms();
    LOG_INFO("[限流器] 令牌桶初始化完成 (容量: %d, 速率: %d/s)", capacity, rate);
}

int token_bucket_consume(int count) {
    pthread_mutex_lock(&tb_mutex);
    long long now = get_time_ms();
    long long elapsed_ms = now - tb_last_time_ms;

    // 核心逻辑：根据距离上一次更新流逝的时间，按速率补充令牌
    if (elapsed_ms > 0) {
        double added_tokens = (double)elapsed_ms * tb_rate / 1000.0;
        tb_tokens += added_tokens;
        
        // 令牌数不能超过桶的物理容量
        if (tb_tokens > tb_capacity) {
            tb_tokens = tb_capacity;
        }
        tb_last_time_ms = now;
    }

    int allowed = 0;
    // 检查是否有足够的令牌供本次消费
    if (tb_tokens >= count) {
        tb_tokens -= count;
        allowed = 1;
    }
    
    pthread_mutex_unlock(&tb_mutex);
    return allowed;
}
