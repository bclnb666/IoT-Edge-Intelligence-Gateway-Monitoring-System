#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <pthread.h>
#include "logger.h"

// 使用互斥锁保证多线程（IPC线程和Cloud线程）同时写日志时的线程安全
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_logger(void) {
    // 预留的初始化接口
}

void write_log(const char *level, const char *format, ...) {
    time_t now;
    struct tm *tm_now;
    char date_str[32];
    char time_str[32];
    char filename[128];

    // 获取当前时间
    time(&now);
    tm_now = localtime(&now);

    // 格式化日期用于文件名，实现按天滚动 (如: gateway_2026-04-15.log)
    strftime(date_str, sizeof(date_str), "%Y-%m-%d", tm_now);
    snprintf(filename, sizeof(filename), "gateway_%s.log", date_str);

    // 格式化完整时间用于日志行首 (如: 2026-04-15 14:15:30)
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_now);

    pthread_mutex_lock(&log_mutex);

    // 以追加模式打开当天的日志文件
    FILE *fp = fopen(filename, "a");
    if (fp != NULL) {
        // 写入到文件
        fprintf(fp, "[%s] [%s] ", time_str, level);
        
        va_list args;
        va_start(args, format);
        vfprintf(fp, format, args);
        va_end(args);
        
        fprintf(fp, "\n");
        fclose(fp);
    }

    // 为了方便调试，同时也将日志输出到终端控制台
    printf("[%s] [%s] ", time_str, level);
    va_list args_console;
    va_start(args_console, format);
    vprintf(format, args_console);
    va_end(args_console);
    printf("\n");

    pthread_mutex_unlock(&log_mutex);
}
