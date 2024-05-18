// logger.h
#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>

// 日志级别定义
typedef enum {
    LOG_DEBUG,   // 调试信息
    LOG_INFO,    // 一般信息
    LOG_WARNING, // 警告信息
    LOG_ERROR    // 错误信息
} LogLevel;

// 初始化日志系统
void log_init(const char *filename);

// 写日志的函数
void log_write(LogLevel level, const char *file, int line, const char *fmt, ...);

// 关闭日志系统
void log_close(void);

// 宏定义，用于简化日志记录操作
#define LOG_DEBUG(fmt, ...) log_write(LOG_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  log_write(LOG_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...) log_write(LOG_WARNING, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) log_write(LOG_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#endif // LOGGER_H
