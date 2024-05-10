#include "server.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <IP> <Port> <ThreadPoolSize>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);
    int thread_pool_size = atoi(argv[3]);

    // Initialize and run the server
    server_init_and_run(ip, port, thread_pool_size);

    return EXIT_SUCCESS;
}

