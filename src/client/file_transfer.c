// file_transfer.c
#include "file_transfer.h"
#include "logger.h"
#include <inttypes.h>

void process_command(int sockfd, const char *command, char *buffer) {
    char cmd[100], local_filename[256];
    int cmd_result = sscanf(command, "%s %s", cmd, local_filename);
    // 下面两行为新加的
    // off_t offset = 0;
    // int cmd_result = sscanf(command, "%s %s %s %ld", cmd, local_filename, remote_filename, &offset);

    LOG_DEBUG("cmd=%s, local_filename=%s", cmd, local_filename);
    if (strcmp(cmd, "gets") == 0 && cmd_result == 2) {
        LOG_INFO("处理gets命令");
        // int file_fd = open(local_filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        char buffer[BUFSIZ];
        ssize_t bytes_received, bytes_written;
        FILE *file;
        off_t file_size, received_size = 0;

        // 接收文件大小
        while ((bytes_received = recv(sockfd, &file_size, sizeof(file_size), 0)) == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                LOG_DEBUG("资源暂时不可用，重试recv");
                continue;
            } else {
                LOG_ERROR("接收文件大小失败，bytes_received=%zd, 错误信息: %s", bytes_received, strerror(errno));
                return;
            }
        }

        LOG_DEBUG("%s的大小为:%jd", local_filename, (intmax_t)file_size);

        if (file_size <= 0) {
            LOG_ERROR("File not found or empty:%jd", (intmax_t)file_size);
            printf("文件未找到或为空.\n");
            return;
        }

        // 检查是否已有部分文件
        file = fopen(local_filename, "rb");
        if (file) {
            fseeko(file, 0, SEEK_END);
            received_size = ftello(file);
            LOG_INFO("已有文件大小:%jd", (intmax_t)received_size);
            fclose(file);
        }

        // 发送当前已接收大小以实现断点续传
        send(sockfd, &received_size, sizeof(received_size), 0);

        // 打开文件以追加
        file = fopen(local_filename, "ab");
        if (!file) {
            LOG_ERROR("打开文件失败");
            perror("fopen");
            return;
        }
        // 接收文件数据
        LOG_INFO("开始接收文件数据");
        off_t total_received = received_size;
        while (total_received < file_size) {
            while ((bytes_received = recv(sockfd, buffer, sizeof(buffer), 0)) == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    LOG_DEBUG("资源暂时不可用，等待数据变得可用");
                    usleep(100); // 等待一小段时间，避免忙等
                    continue;
                } else {
                    LOG_ERROR("接收文件数据失败，bytes_received=%zd, 错误信息: %s", bytes_received, strerror(errno));
                    fclose(file);
                    return;
                }
            }
            if (bytes_received == 0) {
                LOG_DEBUG("服务器关闭连接");
                break;
            }
            bytes_written = fwrite(buffer, 1, bytes_received, file);
            if (bytes_written != bytes_received) {
                perror("fwrite");
                break;
            }
            total_received += bytes_received;
        }

        fclose(file);
        LOG_INFO("文件下载成功: %jd字节.", (intmax_t)total_received);
        printf("文件下载成功:%jd字节.\n", (intmax_t)total_received);
    } else if (strcmp(cmd, "puts") == 0 && cmd_result == 2) {
        LOG_INFO("处理puts命令.");
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
        LOG_INFO("处理其他命令.");
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

