// config.h
#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    char *ip;
    int port;
    int min_threads;
    int max_threads;
    int queue_size;
} ServerConfig;


int read_config(const char *filename, ServerConfig *config);
void free_config(ServerConfig *config);

#endif
