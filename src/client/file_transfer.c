#include "file_transfer.h"
#include "logger.h"
#include "utils.h"
#include "common.h"
#include "ui.h"
#include <inttypes.h>
#include <sys/mman.h>
#include <json-c/json.h>

#define THRESHOLD 10 * 1024 * 1024
#define BUFFER_SIZE 8192

void process_command(int sockfd, const char *command) {
    char cmd[100], local_filename[256];
    bzero(cmd, sizeof(cmd));
    bzero(local_filename, sizeof(local_filename));

    int cmd_result = sscanf(command, "%s %s", cmd, local_filename);

    LOG_DEBUG("cmd=%s, local_filename=%s", cmd, local_filename);
    if (strcmp(cmd, "gets") == 0 && cmd_result == 2) {
        handle_gets(sockfd, local_filename);
    } else if (strcmp(cmd, "puts") == 0 && cmd_result == 2) {
        handle_puts(sockfd, local_filename);
    } else if (strcmp(cmd, "cd") == 0 && cmd_result == 2) {
        handle_cd_response(sockfd);
    } else if (strcmp(cmd, "ls") == 0) {
        handle_ls_response(sockfd);
    } else if (strcmp(cmd, "pwd") == 0) {
        handle_pwd_response(sockfd);
    } else if (strcmp(cmd, "mkdir") == 0) {
        handle_mkdir_response(sockfd);
    } else if (strcmp(cmd, "rm") == 0) {
        handle_remove_response(sockfd);
    }
}


void handle_gets(int sockfd, const char *local_filename) {
    LOG_INFO("处理gets命令");
    char buffer[BUFFER_SIZE];
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
}

void handle_puts(int sockfd, const char *local_filename) {
    LOG_INFO("处理puts命令.");
    char buffer[BUFFER_SIZE];
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
    ssize_t bytes_read, bytes_sent;
    ssize_t total_sent = received_size;
    LOG_INFO("开始传输数据");

    if (file_size >= THRESHOLD) {
        LOG_INFO("大文件MMAP优化");
        void *file_map = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (file_map == MAP_FAILED) {
            LOG_ERROR("文件映射失败");
            perror("mmap");
            return;
        }

        char *data_ptr = (char *)file_map + received_size;

        while (total_sent < file_size) {
            bytes_sent = send(sockfd, data_ptr + total_sent, file_size - total_sent, MSG_NOSIGNAL);
            if (bytes_sent == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                } else {
                    LOG_ERROR("发送文件数据出错:%s", strerror(errno));
                    perror("send");
                    munmap(file_map, file_size);
                    return;
                }
            }
            total_sent += bytes_sent;
        }
        munmap(file_map, file_size);
    } else {
        off_t debug = lseek(fd, received_size, SEEK_SET);
        LOG_DEBUG("偏移成功:%jd", (intmax_t)debug);

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
    }

    close(fd);
    LOG_INFO("文件传输完成，总共发送了%zd字节数据", total_sent);
}

// 接收完整的响应
ssize_t recv_full_response(int server_fd, char *buffer, size_t buffer_size) {
    ssize_t total_bytes_received = 0;
    ssize_t bytes_received;

    while ((bytes_received = recv(server_fd, buffer + total_bytes_received, buffer_size - total_bytes_received - 1, 0)) > 0) {
        total_bytes_received += bytes_received;
        if (strstr(buffer, END_OF_MESSAGE) != NULL) {
            break;
        }
    }

    if (bytes_received == -1) {
        perror("recv");
        return -1;
    }

    buffer[total_bytes_received] = '\0';
    return total_bytes_received;
}

// 处理 pwd 响应
void handle_pwd_response(int server_fd) {
    char buffer[BUFSIZ];
    ssize_t bytes_received = recv_full_response(server_fd, buffer, sizeof(buffer));

    if (bytes_received <= 0) {
        return;
    }

    json_object *json_response = json_tokener_parse(buffer);
    if (json_response == NULL) {
        fprintf(stderr, "Failed to parse JSON response\n");
        return;
    }

    json_object *current_path_obj;
    if (json_object_object_get_ex(json_response, "current_path", &current_path_obj)) {
        const char *current_path = json_object_get_string(current_path_obj);
        printf("Current path: %s\n", current_path);
    } else {
        fprintf(stderr, "Failed to get current path from response\n");
    }

    json_object_put(json_response);
}

// 处理 cd 响应
void handle_cd_response(int server_fd) {
    LOG_DEBUG("处理cd响应");
    char buffer[BUFSIZ];
    ssize_t bytes_received = recv_full_response(server_fd, buffer, sizeof(buffer));

    if (bytes_received <= 0) {
        LOG_ERROR("Failed to receive full response or connection closed");
        return;
    }

    LOG_DEBUG("接收到的响应: %s", buffer);

    json_object *json_response = json_tokener_parse(buffer);
    if (json_response == NULL) {
        LOG_ERROR("Failed to parse JSON response: %s", buffer);
        return;
    }

    json_object *success_obj;
    if (json_object_object_get_ex(json_response, "success", &success_obj) && json_object_get_boolean(success_obj)) {
        json_object *new_path_obj;
        if (json_object_object_get_ex(json_response, "new_path", &new_path_obj)) {
            const char *new_path = json_object_get_string(new_path_obj);
            if (new_path != NULL) {
                LOG_DEBUG("解析到的新路径: %s", new_path);
                strncpy(current_path, new_path,  1023);
                current_path[1023] = '\0'; // 确保字符串以null结尾
                printf("Changed directory to: %s\n", new_path);
            } else {
                LOG_ERROR("new_path is NULL");
            }
        } else {
            LOG_ERROR("new_path field not found in JSON response");
        }
    } else {
        LOG_ERROR("Failed to change directory or success field is false");
        fprintf(stderr, "Failed to change directory\n");
    }

    json_object_put(json_response);
}


// 处理 ls 响应
void handle_ls_response(int server_fd) {
    char buffer[BUFSIZ];
    ssize_t bytes_received = recv_full_response(server_fd, buffer, sizeof(buffer));

    if (bytes_received <= 0) {
        LOG_ERROR("Failed to receive full response or connection closed");
        return;
    }

    LOG_DEBUG("接收到的响应: %s", buffer);

    json_object *json_response = json_tokener_parse(buffer);
    if (json_response == NULL) {
        LOG_ERROR("Failed to parse JSON response: %s", buffer);
        return;
    }

    if (!json_object_is_type(json_response, json_type_array)) {
        LOG_ERROR("JSON response is not an array");
        json_object_put(json_response);
        return;
    }

    json_object *file_entry;
    size_t array_len = json_object_array_length(json_response);
    LOG_DEBUG("JSON array length: %zu", array_len);

    for (size_t i = 0; i < array_len; i++) {
        file_entry = json_object_array_get_idx(json_response, i);
        if (file_entry == NULL || !json_object_is_type(file_entry, json_type_object)) {
            LOG_ERROR("Invalid file entry at index %zu", i);
            continue;
        }

        json_object *filename_obj = json_object_object_get(file_entry, "filename");
        json_object *type_obj = json_object_object_get(file_entry, "type");

        if (filename_obj == NULL || type_obj == NULL) {
            LOG_ERROR("Filename or type not found in file entry at index %zu", i);
            continue;
        }

        const char *filename = json_object_get_string(filename_obj);
        const char *type = json_object_get_string(type_obj);
        printf("%s (%s)\n", filename, type);
    }

    json_object_put(json_response);
}

// 处理 mkdir 响应
void handle_mkdir_response(int server_fd) {
    char buffer[BUFSIZ];
    ssize_t bytes_received = recv_full_response(server_fd, buffer, sizeof(buffer));

    if (bytes_received <= 0) {
        return;
    }

    json_object *json_response = json_tokener_parse(buffer);
    if (json_response == NULL) {
        fprintf(stderr, "Failed to parse JSON response\n");
        return;
    }

    json_object *success_obj;
    if (json_object_object_get_ex(json_response, "success", &success_obj) && json_object_get_boolean(success_obj)) {
        printf("Directory created successfully.\n");
    } else {
        fprintf(stderr, "Failed to create directory.\n");
    }

    json_object_put(json_response);
}

// 处理 remove 响应
void handle_remove_response(int server_fd) {
    char buffer[BUFSIZ];
    ssize_t bytes_received = recv_full_response(server_fd, buffer, sizeof(buffer));

    if (bytes_received <= 0) {
        return;
    }

    json_object *json_response = json_tokener_parse(buffer);
    if (json_response == NULL) {
        fprintf(stderr, "Failed to parse JSON response\n");
        return;
    }

    json_object *success_obj;
    if (json_object_object_get_ex(json_response, "success", &success_obj) && json_object_get_boolean(success_obj)) {
        printf("File removed successfully.\n");
    } else {
        fprintf(stderr, "Failed to remove file.\n");
    }

    json_object_put(json_response);
}
