#include "server.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <config_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    ServerConfig config;
    if (read_config(argv[1], &config) != 0) {
        fprintf(stderr, "Failed to read configuration file.\n");
        return EXIT_FAILURE;
    }

    // Initialize and run the server
    server_init_and_run(config.ip, config.port, config.min_threads, 
                        config.max_threads, config.queue_size);
    
    free_config(&config);

    return EXIT_SUCCESS;
}

