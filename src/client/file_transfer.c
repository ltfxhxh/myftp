// file_transfer.c
#include "file_transfer.h"

void process_command(int sockfd, const char *command, char *buffer) {
    char cmd[100], local_filename[256], remote_filename[256];
    int cmd_result = sscanf(command, "%s %s %s", cmd, local_filename, remote_filename);

    if (strcmp(cmd, "gets") == 0 && cmd_result == 3) {
        int file_fd = open(local_filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (file_fd < 0) {
            perror("Error opening local file for writing");
            return;
        }

        ssize_t received;
        int total_received = 0;
        while (1) {
            received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
            if (received > 0) {
                buffer[received] = '\0';
                if (strstr(buffer, END_OF_MESSAGE)) {
                    *strstr(buffer, END_OF_MESSAGE) = '\0';
                    if (write(file_fd, buffer, strlen(buffer)) != (ssize_t)strlen(buffer)) {
                        perror("Error writing last part to local file");
                    }
                    break;
                }
                if (write(file_fd, buffer, received) != received) {
                    perror("Error writing to local file");
                    close(file_fd);
                    return;
                }
                total_received += received;
            } else if (received == 0) {
                break;
            } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("recv failed");
                break;
            }
        }
        printf(ANSI_COLOR_GREEN "Received %d bytes and saved to '%s'.\n" ANSI_COLOR_RESET, total_received, local_filename);
        close(file_fd);
    } else if (strcmp(cmd, "puts") == 0 && cmd_result == 3) {
        int file_fd = open(local_filename, O_RDONLY);
        if (file_fd < 0) {
            perror("Error opening local file for reading");
            return;
        }

        ssize_t read_bytes;
        int total_sent = 0;
        while ((read_bytes = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
            if (send(sockfd, buffer, read_bytes, 0) != read_bytes) {
                perror("Error sending file data to server");
                close(file_fd);
                return;
            }
            total_sent += read_bytes;
        }
        if (read_bytes < 0) {
            perror("Error reading local file");
        } else {
            printf(ANSI_COLOR_GREEN "Sent %d bytes from '%s' to server.\n" ANSI_COLOR_RESET, total_sent, local_filename);
        }
        close(file_fd);
    } else {
        ssize_t received;
        int total_received = 0;
        while (1) {
            received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
            if (received > 0) {
                buffer[received] = '\0';
                if (strstr(buffer, END_OF_MESSAGE)) {
                    *strstr(buffer, END_OF_MESSAGE) = '\0';
                    printf(ANSI_COLOR_BLUE "%s" ANSI_COLOR_RESET, buffer);
                    break;
                }
                printf(ANSI_COLOR_BLUE "%s" ANSI_COLOR_RESET, buffer);
                total_received += received;
            } else if (received == 0) {
                break;
            } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("recv failed");
                break;
            }
        }
    }
}

