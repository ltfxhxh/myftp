#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} LogLevel;

// 初始化日志系统
void logger_init(const char *log_file);

// 记录日志
void logger_log(LogLevel level, const char *format, ...);

#endif // LOGGER_H

