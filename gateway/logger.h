#ifndef __LOGGER_H__
#define __LOGGER_H__

// 初始化日志互斥锁（虽然在单文件直接调用中可以使用静态锁，但对外暴露接口是个好习惯）
void init_logger(void);

// 核心日志写入函数
void write_log(const char *level, const char *format, ...);

// 方便使用的宏定义，会自动带上日志级别
#define LOG_INFO(fmt, ...)  write_log("INFO ", fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  write_log("WARN ", fmt, ##__VA_ARGS__)
#define LOG_ERR(fmt, ...)   write_log("ERROR", fmt, ##__VA_ARGS__)

#endif // __LOGGER_H__
