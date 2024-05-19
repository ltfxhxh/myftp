#include "server.h"
#include "config.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    // 初始化日志系统
    log_init("../logs/server.log");
    LOG_INFO("server starting...");

    if (argc < 2) {
        LOG_ERROR("Usage: %s <config_file>", argv[0]);
        fprintf(stderr, "Usage: %s <config_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    ServerConfig config;
    if (read_config(argv[1], &config) != 0) {
        LOG_ERROR("Failed to read configuration file: %s", argv[1]);
        fprintf(stderr, "Failed to read configuration file.\n");
        return EXIT_FAILURE;
    }

    LOG_INFO("Configuration loaded: IP=%s, Port=%d, MinThreads=%d, MaxThreads=%d, QueueSize=%d", 
             config.ip, config.port, config.min_threads, config.max_threads, config.queue_size);

    // 初始化并运行服务器
    server_init_and_run(config.ip, config.port, config.min_threads, 
                        config.max_threads, config.queue_size);
    
    free_config(&config);
    
    LOG_INFO("Server shut down.");
    log_close();

    return EXIT_SUCCESS;
}

