#include "config.h"
#include <libconfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int read_config(const char *filename, ServerConfig *config) {
    config_t cfg;
    config_init(&cfg);
    printf("Attempting to read config file: %s\n", filename); // 添加调试信息
    if (!config_read_file(&cfg, filename)) {
        fprintf(stderr, "Error reading config file %s: %s at line %d - %s\n", filename, 
                config_error_text(&cfg),
                config_error_line(&cfg),
                config_error_file(&cfg));
        config_destroy(&cfg);
        return -1;
    } 

    const char *ip;
    if (config_lookup_string(&cfg, "server.ip", &ip)) {
        config->ip = strdup(ip);
    } else {
        fprintf(stderr, "No 'ip' setting in configuration file.\n");
        config_destroy(&cfg);
        return -1;
    }

    if (!config_lookup_int(&cfg, "server.port", &config->port)) {
        fprintf(stderr, "No 'port' setting in configuration file.\n");
        config_destroy(&cfg);
        return -1;
    }

    if (!config_lookup_int(&cfg, "server.min_threads", &config->min_threads)) {
        fprintf(stderr, "No 'min_threads' setting in configuration file.\n");
        config_destroy(&cfg);
        return -1;
    }

    if (!config_lookup_int(&cfg, "server.max_threads", &config->max_threads)) {
        fprintf(stderr, "No 'max_threads' setting in configuration file.\n");
        config_destroy(&cfg);
        return -1;
    }

    if (!config_lookup_int(&cfg, "server.queue_size", &config->queue_size)) {
        fprintf(stderr, "No 'queue_size' setting in configuration file.\n");
        config_destroy(&cfg);
        return -1;
    }

    config_destroy(&cfg);
    return 0;
}

void free_config(ServerConfig *config) {
    if (config->ip) {
        free(config->ip);
    }
}

