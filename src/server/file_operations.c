#include "file_operations.h"
#include "logger.h"  // Include the logger header
#include "network_utils.h"
#include "epoll_manager.h"
#include "network_utils.h"
#include "database.h"
#include <json-c/json.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <errno.h>

#define THRESHOLD 100 * 1024 * 1024

const char *END_OF_MESSAGE = "\n.\n";

off_t get_file_size(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == 0) {
        return st.st_size;
    } else {
        LOG_ERROR("stat failed for file: %s", filename);
        return -1;
    }
}

void handle_pwd(int client_fd, int user_id, MYSQL *conn) {
    LOG_DEBUG("处理pwd命令，user_id=%d", user_id);
    char current_path[256];
    
    // 从数据库中获取用户当前路径
    if (get_user_current_path(conn, user_id, current_path) != 0) {
        LOG_ERROR("获取当前目录失败，user_id=%d", user_id);
        json_object *response = json_object_new_object();
        json_object_object_add(response, "error", json_object_new_string("Failed to get current directory."));
        const char *response_str = json_object_to_json_string(response);
        write(client_fd, response_str, strlen(response_str));
        write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
        json_object_put(response);
        return;
    }

    LOG_DEBUG("当前目录：%s", current_path);

    // 创建 JSON 响应对象
    json_object *response = json_object_new_object();
    json_object_object_add(response, "current_path", json_object_new_string(current_path));

    // 将 JSON 响应对象转换为字符串
    const char *response_str = json_object_to_json_string(response);

    // 将响应发送给客户端
    write(client_fd, response_str, strlen(response_str));
    write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));

    // 释放 JSON 对象
    json_object_put(response);
}

// 处理cd命令


void handle_cd(int client_fd, const char *path, int user_id, MYSQL *conn) {
    LOG_DEBUG("处理cd命令，user_id=%d，path=%s", user_id, path);
    char current_path[256];

    // 获取用户当前路径
    if (get_user_current_path(conn, user_id, current_path) != 0) {
        LOG_ERROR("获取当前目录失败，user_id=%d", user_id);
        json_object *response = json_object_new_object();
        json_object_object_add(response, "error", json_object_new_string("Failed to get current directory."));
        const char *response_str = json_object_to_json_string(response);
        write(client_fd, response_str, strlen(response_str));
        write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
        json_object_put(response);
        return;
    }

    LOG_DEBUG("当前目录：%s", current_path);

    char new_path[256];

    // 处理 ".." 路径
    if (strcmp(path, "..") == 0) {
        LOG_DEBUG("处理..路径");
        if (strcmp(current_path, "/") == 0) {
            // 当前目录为根目录时，不做任何操作
            snprintf(new_path, sizeof(new_path), "%s", current_path);
        } else {
            char *last_slash = strrchr(current_path, '/');
            if (last_slash != NULL) {
                *last_slash = '\0';
                if (strlen(current_path) == 0) {
                    strcpy(current_path, "/");
                }
            }
            snprintf(new_path, sizeof(new_path), "%s", current_path);
        }
    } else {
        // 处理其他路径
        LOG_DEBUG("处理其他路径");
        if (strcmp(current_path, "/") == 0) {
            LOG_DEBUG("当前路径为根目录");
            snprintf(new_path, sizeof(new_path), "/%s", path);
        } else {
            LOG_DEBUG("当前路径不为根目录");
            snprintf(new_path, sizeof(new_path), "%s/%s", current_path, path);
        }
    }

    LOG_DEBUG("新的路径：%s", new_path);

    // 如果新的路径是根目录，不查表，直接更新
    if (strcmp(new_path, "/") == 0) {
        if (update_user_current_path(conn, user_id, new_path) != 0) {
            LOG_ERROR("更新目录失败：%s", new_path);
            json_object *response = json_object_new_object();
            json_object_object_add(response, "error", json_object_new_string("Failed to change directory."));
            const char *response_str = json_object_to_json_string(response);
            write(client_fd, response_str, strlen(response_str));
            write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
            json_object_put(response);
            return;
        }
    } else {
        // 检查新路径是否存在于虚拟文件表中
        if (!path_exists_in_vft(conn, user_id, new_path)) {
            LOG_ERROR("目录不存在：%s", new_path);
            json_object *response = json_object_new_object();
            json_object_object_add(response, "error", json_object_new_string("Directory does not exist."));
            const char *response_str = json_object_to_json_string(response);
            write(client_fd, response_str, strlen(response_str));
            write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
            json_object_put(response);
            return;
        }

        // 更新用户当前路径
        if (update_user_current_path(conn, user_id, new_path) != 0) {
            LOG_ERROR("更新目录失败：%s", new_path);
            json_object *response = json_object_new_object();
            json_object_object_add(response, "error", json_object_new_string("Failed to change directory."));
            const char *response_str = json_object_to_json_string(response);
            write(client_fd, response_str, strlen(response_str));
            write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
            json_object_put(response);
            return;
        }
    }

    // 将新的路径返回给客户端
    json_object *response = json_object_new_object();
    json_object_object_add(response, "success", json_object_new_boolean(1));
    json_object_object_add(response, "new_path", json_object_new_string(new_path));

    const char *response_str = json_object_to_json_string(response);
    write(client_fd, response_str, strlen(response_str));
    write(client_fd, END_OF_MESSAGE, strlen(response_str));

    json_object_put(response);
}


// 处理mkdir命令
void handle_mkdir(int client_fd, const char *dirname, int user_id, MYSQL *conn) {
    char current_path[256];

    // 获取用户当前路径
    if (get_user_current_path(conn, user_id, current_path) != 0) {
        json_object *response = json_object_new_object();
        json_object_object_add(response, "success", json_object_new_boolean(0));
        json_object_object_add(response, "message", json_object_new_string("Failed to get current directory."));
        const char *response_str = json_object_to_json_string(response);
        write(client_fd, response_str, strlen(response_str));
        write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
        json_object_put(response);
        return;
    }

    // 获取当前路径的ID
    int parent_id = get_directory_id(conn, user_id, current_path);
    if (parent_id == -1) {
        json_object *response = json_object_new_object();
        json_object_object_add(response, "success", json_object_new_boolean(0));
        json_object_object_add(response, "message", json_object_new_string("Failed to get parent directory ID."));
        const char *response_str = json_object_to_json_string(response);
        write(client_fd, response_str, strlen(response_str));
        write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
        json_object_put(response);
        return;
    }

    // 检查当前路径下是否存在同名目录
    if (directory_exists_in_vft(conn, user_id, current_path, dirname)) {
        json_object *response = json_object_new_object();
        json_object_object_add(response, "success", json_object_new_boolean(0));
        json_object_object_add(response, "message", json_object_new_string("Directory already exists."));
        const char *response_str = json_object_to_json_string(response);
        write(client_fd, response_str, strlen(response_str));
        write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
        json_object_put(response);
        return;
    }

    char full_path[512];
    if (strcmp(current_path, "/") == 0) {
        snprintf(full_path, sizeof(full_path), "/%s", dirname);
    } else {
        snprintf(full_path, sizeof(full_path), "%s/%s", current_path, dirname);
    }

    // 在虚拟文件表中插入新目录信息
    if (insert_directory_into_vft(conn, user_id, dirname, full_path, parent_id) != 0) {
        json_object *response = json_object_new_object();
        json_object_object_add(response, "success", json_object_new_boolean(0));
        json_object_object_add(response, "message", json_object_new_string("Failed to create directory."));
        const char *response_str = json_object_to_json_string(response);
        write(client_fd, response_str, strlen(response_str));
        write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
        json_object_put(response);
        return;
    }

    json_object *response = json_object_new_object();
    json_object_object_add(response, "success", json_object_new_boolean(1));
    json_object_object_add(response, "message", json_object_new_string("Directory created successfully."));
    const char *response_str = json_object_to_json_string(response);
    write(client_fd, response_str, strlen(response_str));
    write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
    json_object_put(response);
}

void handle_ls(int client_fd, int user_id, MYSQL *conn) {
    LOG_DEBUG("处理ls命令，user_id=%d", user_id);
    char current_path[256];

    // 获取用户当前路径
    if (get_user_current_path(conn, user_id, current_path) != 0) {
        LOG_ERROR("获取当前目录失败，user_id=%d", user_id);
        json_object *response = json_object_new_object();
        json_object_object_add(response, "error", json_object_new_string("Failed to get current directory."));
        const char *response_str = json_object_to_json_string(response);
        write(client_fd, response_str, strlen(response_str));
        write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
        json_object_put(response);
        return;
    }

    LOG_DEBUG("获取到的当前路径：%s", current_path);

    // 获取当前路径的ID
    int parent_id = get_directory_id(conn, user_id, current_path);
    if (parent_id == -1) {
        LOG_ERROR("获取当前目录ID失败，user_id=%d", user_id);
        json_object *response = json_object_new_object();
        json_object_object_add(response, "error", json_object_new_string("Failed to get current directory ID."));
        const char *response_str = json_object_to_json_string(response);
        write(client_fd, response_str, strlen(response_str));
        write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
        json_object_put(response);
        return;
    }

    // 构建 SQL 查询语句以获取当前路径下的文件和目录
    char query[512];
    snprintf(query, sizeof(query), "SELECT filename, type FROM virtual_file_table WHERE owner_id = %d AND parent_id = %d", user_id, parent_id);

    // 执行查询
    if (mysql_query(conn, query)) {
        LOG_ERROR("SELECT * FROM virtual_file_table failed: %s", mysql_error(conn));
        json_object *response = json_object_new_object();
        json_object_object_add(response, "error", json_object_new_string("Failed to list directory contents."));
        const char *response_str = json_object_to_json_string(response);
        write(client_fd, response_str, strlen(response_str));
        write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
        json_object_put(response);
        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (res == NULL) {
        LOG_ERROR("mysql_store_result() failed: %s", mysql_error(conn));
        json_object *response = json_object_new_object();
        json_object_object_add(response, "error", json_object_new_string("Failed to retrieve directory contents."));
        const char *response_str = json_object_to_json_string(response);
        write(client_fd, response_str, strlen(response_str));
        write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
        json_object_put(response);
        return;
    }

    // 处理查询结果并构建 JSON 响应
    MYSQL_ROW row;
    json_object *response = json_object_new_array();
    int row_count = 0;
    while ((row = mysql_fetch_row(res)) != NULL) {
        json_object *file_entry = json_object_new_object();
        json_object_object_add(file_entry, "filename", json_object_new_string(row[0]));
        json_object_object_add(file_entry, "type", json_object_new_string(row[1]));
        json_object_array_add(response, file_entry);
        row_count++;
    }
    LOG_DEBUG("查询到的行数：%d", row_count);

    // 将 JSON 响应转换为字符串并发送给客户端
    const char *response_str = json_object_to_json_string(response);
    LOG_DEBUG("发送的 JSON 响应：%s", response_str);
    write(client_fd, response_str, strlen(response_str));
    write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
    json_object_put(response);

    // 释放查询结果
    mysql_free_result(res);
}


void handle_puts(int client_fd, const char *filename, int user_id, MYSQL *conn) {
    LOG_DEBUG("处理puts命令，user_id=%d，filename=%s", user_id, filename);

    char buffer[BUFSIZ];
    ssize_t bytes_received, bytes_written;
    FILE *file;
    off_t file_size, received_size = 0;

    // 接收文件大小
    bytes_received = recv(client_fd, &file_size, sizeof(file_size), 0);
    if (bytes_received == 0) {
        LOG_ERROR("客户端关闭连接");
        return;
    } else if (bytes_received == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            LOG_DEBUG("资源暂时不可用，重试recv");
            return;
        } else {
            LOG_ERROR("接收文件大小失败，错误信息：%s", strerror(errno));
            return;
        }
    }

    LOG_DEBUG("%s的大小为：%jd", filename, (intmax_t)file_size);

    if (file_size <= 0) {
        LOG_ERROR("文件不存在或为空：%jd", (intmax_t)file_size);
        return;
    }

    char current_path[256];
    if (get_user_current_path(conn, user_id, current_path) != 0) {
        LOG_ERROR("获取当前目录失败，user_id=%d", user_id);
        return;
    }

    char full_path[512];
    snprintf(full_path, sizeof(full_path), "storage/%s", filename);

    // 检查是否已有部分文件
    file = fopen(full_path, "rb");
    if (file) {
        fseeko(file, 0, SEEK_END);
        received_size = ftello(file);
        LOG_INFO("已有文件大小：%jd", (intmax_t)received_size);
        fclose(file);
    } else {
        LOG_DEBUG("没有部分文件");
        received_size = 0;
    }

    // 发送当前已接收大小以实现断点续传
    send(client_fd, &received_size, sizeof(received_size), MSG_NOSIGNAL);
    LOG_DEBUG("发送当前已接收大小以实现断点续传：%jd", (intmax_t)received_size);

    // 打开文件以追加
    file = fopen(full_path, "ab");
    if (!file) {
        LOG_ERROR("打开文件失败");
        perror("fopen");
        return;
    }
    LOG_DEBUG("成功打开文件");

    // 接收文件数据
    LOG_INFO("开始接收文件数据");
    off_t total_received = received_size;
    while (total_received < file_size) {
        bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes_received == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                LOG_DEBUG("资源暂时不可用，等待数据变得可用");
                usleep(100);
                continue;
            } else {
                LOG_ERROR("接收文件数据失败，bytes_received=%zd，错误信息：%s", bytes_received, strerror(errno));
                fclose(file);
                return;
            }
        } else if (bytes_received == 0) {
            LOG_DEBUG("客户端关闭连接");
            break;
        }
        bytes_written = fwrite(buffer, 1, bytes_received, file);
        if (bytes_written != bytes_received) {
            LOG_ERROR("读到%zd字节，写入%zd字节", bytes_received, bytes_written);
            perror("fwrite");
            break;
        }
        total_received += bytes_received;
    }

    fclose(file);
    LOG_INFO("文件下载成功：%jd字节。", (intmax_t)total_received);

    // 将文件信息插入到虚拟文件表中
    if (insert_file_into_vft(conn, user_id, filename, current_path) != 0) {
        LOG_ERROR("将文件插入虚拟文件表失败：%s", filename);
    }
}

void handle_gets(int client_fd, const char *filename, int user_id, MYSQL *conn) {
    LOG_DEBUG("处理gets命令，user_id=%d，filename=%s", user_id, filename);

    char buffer[BUFSIZ];
    int fd;

    char current_path[256];
    if (get_user_current_path(conn, user_id, current_path) != 0) {
        LOG_ERROR("获取当前目录失败，user_id=%d", user_id);
        return;
    }

    // 从虚拟文件表中查找文件
    char virtual_path[512];
    snprintf(virtual_path, sizeof(virtual_path), "%s/%s", current_path, filename);
    if (!path_exists_in_vft(conn, user_id, virtual_path)) {
        LOG_ERROR("文件不存在于虚拟文件表中：%s", virtual_path);
        off_t file_size = -1;
        send(client_fd, &file_size, sizeof(file_size), 0);
        return;
    }

    char full_path[512];
    snprintf(full_path, sizeof(full_path), "storage/%s", filename);

    off_t file_size = get_file_size(full_path);
    LOG_INFO("%s的大小：%jd", full_path, (intmax_t)file_size);
    if (file_size == -1) {
        LOG_ERROR("文件不存在");
        file_size = 0;
        send(client_fd, &file_size, sizeof(file_size), 0);
        LOG_DEBUG("文件不存在后发送文件大小：%jd", (intmax_t)file_size);
        return;
    }

    send(client_fd, &file_size, sizeof(file_size), 0);
    LOG_INFO("传输文件大小给客户端：%jd", (intmax_t)file_size);

    fd = open(full_path, O_RDONLY);
    if (fd == -1) {
        LOG_ERROR("打开%s失败", full_path);
        perror("open");
        return;
    }
    LOG_INFO("文件打开成功：%s", full_path);

    off_t received_size = 0;
    recv(client_fd, &received_size, sizeof(received_size), 0);
    LOG_INFO("收到偏移量：%jd", (intmax_t)received_size);
    lseek(fd, received_size, SEEK_SET);
    LOG_DEBUG("偏移成功：%jd", (intmax_t)received_size);

    // 判断是否为大文件传输
    if (file_size - received_size >= THRESHOLD) {
        off_t remaining_size = file_size - received_size;
        LOG_INFO("大文件传输");
        sendfile(client_fd, fd, NULL, remaining_size);
    } else {
        // 发送文件数据
        ssize_t bytes_read;
        LOG_INFO("开始传输数据");
        while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
            if (send(client_fd, buffer, bytes_read, 0) == -1) {
                LOG_ERROR("发送文件数据出错");
                perror("send");
                break;
            }
        }
    }

    close(fd);
}

void handle_remove(int client_fd, const char *filename, int user_id, MYSQL *conn) {
    LOG_DEBUG("处理remove命令，user_id=%d，filename=%s", user_id, filename);

    char current_path[256];
    if (get_user_current_path(conn, user_id, current_path) != 0) {
        LOG_ERROR("获取当前目录失败，user_id=%d", user_id);
        json_object *response = json_object_new_object();
        json_object_object_add(response, "success", json_object_new_boolean(0));
        json_object_object_add(response, "message", json_object_new_string("Failed to get current directory."));
        const char *response_str = json_object_to_json_string(response);
        write(client_fd, response_str, strlen(response_str));
        write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
        json_object_put(response);
        return;
    }

    char full_path[512];
    if (strcmp(current_path, "/") == 0) {
        snprintf(full_path, sizeof(full_path), "%s%s", current_path, filename);
    } else {
        snprintf(full_path, sizeof(full_path), "%s/%s", current_path, filename);
    }

    // 获取当前路径的ID
    int parent_id = get_directory_id(conn, user_id, current_path);
    if (parent_id == -1) {
        LOG_ERROR("获取当前目录ID失败，user_id=%d", user_id);
        json_object *response = json_object_new_object();
        json_object_object_add(response, "success", json_object_new_boolean(0));
        json_object_object_add(response, "message", json_object_new_string("Failed to get current directory ID."));
        const char *response_str = json_object_to_json_string(response);
        write(client_fd, response_str, strlen(response_str));
        write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
        json_object_put(response);
        return;
    }

    // 检查文件是否存在于虚拟文件表中
    int file_id = get_file_id(conn, user_id, parent_id, filename);
    if (file_id == -1) {
        LOG_ERROR("文件不存在：%s", full_path);
        json_object *response = json_object_new_object();
        json_object_object_add(response, "success", json_object_new_boolean(0));
        json_object_object_add(response, "message", json_object_new_string("File does not exist."));
        const char *response_str = json_object_to_json_string(response);
        write(client_fd, response_str, strlen(response_str));
        write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
        json_object_put(response);
        return;
    }

    // 获取文件类型
    char file_type = get_file_type(conn, file_id);
    if (file_type == 'f') {
        // 删除实际文件
        if (unlink(full_path) == -1) {
            LOG_ERROR("Failed to remove file: %s", full_path);
            perror("Remove file failed");
            json_object *response = json_object_new_object();
            json_object_object_add(response, "success", json_object_new_boolean(0));
            json_object_object_add(response, "message", json_object_new_string("Can't remove file."));
            const char *response_str = json_object_to_json_string(response);
            write(client_fd, response_str, strlen(response_str));
            write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
            json_object_put(response);
            return;
        }
    }

    // 删除虚拟文件表中的记录
    if (delete_virtual_file_record(conn, file_id) != 0) {
        LOG_ERROR("Failed to remove file record from database: %s", full_path);
        json_object *response = json_object_new_object();
        json_object_object_add(response, "success", json_object_new_boolean(0));
        json_object_object_add(response, "message", json_object_new_string("Can't remove file record."));
        const char *response_str = json_object_to_json_string(response);
        write(client_fd, response_str, strlen(response_str));
        write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
        json_object_put(response);
        return;
    }

    LOG_INFO("File removed successfully: %s", full_path);
    json_object *response = json_object_new_object();
    json_object_object_add(response, "success", json_object_new_boolean(1));
    json_object_object_add(response, "message", json_object_new_string("File removed successfully."));
    const char *response_str = json_object_to_json_string(response);
    write(client_fd, response_str, strlen(response_str));
    write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
    json_object_put(response);
}
