// file_transfer.c
#include "file_transfer.h"
#include "logger.h"
#include "utils.h"
#include <inttypes.h>

void process_command(int sockfd, const char *command, char *buffer) {
    char cmd[100], local_filename[256];
    bzero(cmd, sizeof(cmd));
    bzero(local_filename, sizeof(local_filename));

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
        LOG_DEBUG("处理puts命令.");
        char buffer[BUFSIZ];
        int fd;

        off_t file_size = get_file_size(local_filename);
        LOG_INFO("%s的大小:%jd", local_filename, (intmax_t)file_size);
        if (file_size == -1) {
            LOG_ERROR("文件不存在");
            file_size = 0;
            send(sockfd, &file_size, sizeof(file_size), 0);
            LOG_DEBUG("文件不存在后发送文件大小:%jd", (intmax_t)file_size);
            return;
        }

        send(sockfd, &file_size, sizeof(file_size), MSG_NOSIGNAL);
        LOG_INFO("传输文件大小给客户端:%jd", (intmax_t)file_size);

        fd = open(local_filename, O_RDONLY);
        if (fd == -1) {
            LOG_ERROR("打开%s失败", local_filename);
            perror("open");
            return;
        }
        LOG_INFO("文件打开成功:%s", local_filename);

        off_t received_size = 0;
        while (recv(sockfd, &received_size, sizeof(received_size), 0) == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            } else {
                LOG_ERROR("接收偏移量失败");
                return;
            }
        }
        LOG_INFO("收到偏移量:%jd", (intmax_t)received_size);
        off_t debug = lseek(fd, received_size, SEEK_SET);
        LOG_DEBUG("偏移成功:%jd", (intmax_t)debug);

        ssize_t bytes_read, bytes_sent;
ssize_t total_sent = received_size;
LOG_INFO("开始传输数据");

while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
    ssize_t total_bytes_sent = 0;
    while (total_bytes_sent < bytes_read) {
        bytes_sent = send(sockfd, buffer + total_bytes_sent, bytes_read - total_bytes_sent, MSG_NOSIGNAL);
        if (bytes_sent == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            } else {
                LOG_ERROR("发送文件数据出错:%s", strerror(errno));
                perror("send");
                close(fd);
                return;
            }
        }
        total_bytes_sent += bytes_sent;
    }
    total_sent += total_bytes_sent;
}

if (bytes_read == -1) {
    LOG_ERROR("读取文件出错:%s", strerror(errno));
    perror("read");
}

close(fd);
LOG_INFO("文件传输完成，总共发送了%zd字节数据", total_sent);

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

