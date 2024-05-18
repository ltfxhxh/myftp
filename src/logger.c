// logger.c
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>

static FILE *log_file = NULL;
static pthread_mutex_t log_mutex;

void log_init(const char *filename) {
    if (filename != NULL) {
        log_file = fopen(filename, "a");
        if (log_file == NULL) {
            fprintf(stderr, "Failed to open log file: %s\n", filename);
            exit(EXIT_FAILURE);
        }
    } else {
        log_file = stdout;  // 默认输出到stdout
    }

    pthread_mutex_init(&log_mutex, NULL);
}

void log_write(LogLevel level, const char *file, int line, const char *fmt, ...) {
    const char *level_strings[] = {"DEBUG", "INFO", "WARNING", "ERROR"};
    char time_buffer[64];
    va_list args;
    
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", t);
    
    pthread_mutex_lock(&log_mutex);
    
    fprintf(log_file, "%s [%s] (%s:%d): ", time_buffer, level_strings[level], file, line);
    
    va_start(args, fmt);
    vfprintf(log_file, fmt, args);
    va_end(args);
    
    fprintf(log_file, "\n");
    fflush(log_file);
    
    pthread_mutex_unlock(&log_mutex);
}

void log_close(void) {
    if (log_file != NULL) {
        fclose(log_file);
        log_file = NULL;
    }

    pthread_mutex_destroy(&log_mutex);
}
