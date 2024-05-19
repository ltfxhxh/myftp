// client_main.c
#include "network.h"
#include "ui.h"
#include "utils.h"

const char *END_OF_MESSAGE = "\n.\n";

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <server IP> <server port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    int sockfd = connect_to_server(server_ip, server_port);
    if (sockfd < 0) {
        return EXIT_FAILURE;
    }

    display_welcome_message();

    char command[BUFFER_SIZE];
    char cwd[BUFFER_SIZE];
    while (1) {
        bzero(cwd, BUFFER_SIZE);
        check_cwd(cwd, BUFFER_SIZE);

        printf(ANSI_COLOR_YELLOW "~%s$ " ANSI_COLOR_RESET, cwd);
        fflush(stdout);

        if (fgets(command, BUFFER_SIZE, stdin) == NULL) {
            printf(ANSI_COLOR_RED "Failed to read command.\n" ANSI_COLOR_RESET);
            continue;
        }

        trim_newline(command);

        if (strlen(command) == 0) {
            continue;
        }

        if (strcmp(command, "exit") == 0) {
            printf(ANSI_COLOR_GREEN "Exiting...\n" ANSI_COLOR_RESET);
            break;
        }

        send_command(sockfd, command);
    }

    close(sockfd);
    return EXIT_SUCCESS;
}

