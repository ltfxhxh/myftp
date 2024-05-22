#include "file_operations.h"
#include "logger.h"  // Include the logger header
#include "network_utils.h"
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <inttypes.h>

#define THRESHOLD 100 * 1024 * 1024

off_t get_file_size(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == 0) {
        return st.st_size;
    }
    return -1;
}

const char *END_OF_MESSAGE = "\n.\n";

void handle_cd(int client_fd, const char *path) {
    LOG_DEBUG("Attempting to change directory to: %s", path);
    if (chdir(path) == -1) {
        ERROR_CHECK(client_fd, "切换目录失败.\n");
    } else {
        LOG_INFO("Changed directory to: %s", path);
        const char *msg = "Change Directory Successfully.\n";
        write(client_fd, msg, strlen(msg));
        write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
    }
}

void handle_ls(int client_fd) {
    DIR *dir;
    struct dirent *entry;
    char buffer[1024];
    int buflen = 0;

    LOG_DEBUG("Listing directory contents.");
    if ((dir = opendir(".")) == NULL) {
        ERROR_CHECK(client_fd, "打开目录失败.\n");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        int needed = snprintf(buffer + buflen, sizeof(buffer) - buflen, "%s\n", entry->d_name);
        if (needed < 0 || needed >= sizeof(buffer) - buflen) {
            if (write(client_fd, buffer, buflen) != buflen) {
                LOG_ERROR("Failed to write directory contents.");
                perror("write error");
                closedir(dir);
                return;
            }
            buflen = 0;
            needed = snprintf(buffer, sizeof(buffer), "%s\n", entry->d_name);
            if (needed >= sizeof(buffer)) {
                write(client_fd, entry->d_name, strlen(entry->d_name));
                write(client_fd, "\n", 1);
                continue;
            }
        }
        buflen += needed;
    }
    if (buflen > 0 && write(client_fd, buffer, buflen) != buflen) {
        LOG_ERROR("Failed to write remaining directory contents.");
        perror("write error");
    }
    write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
    closedir(dir);
    LOG_INFO("Directory listing completed.");
}

void handle_puts(int client_fd, const char *filename) {
    LOG_DEBUG("Attempting to store file: %s", filename);
    int file_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (file_fd < 0) {
        ERROR_CHECK(client_fd, "上传失败.\n");
        return;
    }

    char buf[BUFSIZ];
    ssize_t readbytes;
    while ((readbytes = read(client_fd, buf, sizeof(buf))) > 0) {
        if (write(file_fd, buf, readbytes) != readbytes) {
            ERROR_CHECK(client_fd, "上传失败.\n");
            close(file_fd);
            return;
        }
    }

    if (readbytes < 0) {
        LOG_ERROR("Error reading from client during PUTS operation.");
        perror("Read file error");
        const char *msg = "Puts file error occurred in reading file\n";
        write(client_fd, msg, strlen(msg));
    } else {
        LOG_INFO("File stored successfully: %s", filename);
        const char *msg = "Puts file successfully!\n";
        write(client_fd, msg, strlen(msg));
    }
    write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
    close(file_fd);
}

void handle_gets(int client_fd, const char *filename) {
    LOG_DEBUG("处理gets命令");
    char buffer[BUFSIZ];
    int fd;

    off_t file_size = get_file_size(filename);
    LOG_INFO("%s的大小:%jd", filename, (intmax_t)file_size);
    if (file_size == -1) {
        LOG_ERROR("文件不存在");
        file_size = 0;
        send(client_fd, &file_size, sizeof(file_size), 0);
        LOG_DEBUG("文件不存在后发送文件大小:%jd", (intmax_t)file_size);
        return;
    }

    send(client_fd, &file_size, sizeof(file_size), 0);
    LOG_INFO("传输文件大小给客户端:%jd", (intmax_t)file_size);
    
    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        LOG_ERROR("打开%s失败", filename);
        perror("open");
        return;
    }
    LOG_INFO("文件打开成功:%s", filename);

    off_t received_size = 0;
    recv(client_fd, &received_size, sizeof(received_size), 0);
    LOG_INFO("收到偏移量:%jd", (intmax_t)received_size);
    lseek(fd, received_size, SEEK_SET);
    LOG_DEBUG("偏移成功:%jd", (intmax_t)received_size);

    // 判断是否为大文件传输
    if (file_size - received_size >= THRESHOLD) {
        off_t remaining_size = file_size - received_size;
        LOG_INFO("大文件传输");
        sendfile(client_fd, fd, NULL, remaining_size);
    } else {
        // 发送文件数据
        ssize_t bytes_read;
        LOG_INFO("开始传输数据");
        while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0 ) {
            if (send(client_fd, buffer, bytes_read, 0) == -1) {
                LOG_ERROR("发送文件数据出错");
                perror("send");
                break;
            }
        }
    }
    
    close(fd);
    
}

void handle_remove(int client_fd, const char *filename) {
    LOG_DEBUG("Attempting to remove file: %s", filename);
    if (remove(filename) == -1) {
        LOG_ERROR("Failed to remove file: %s", filename);
        perror("Remove file failed");
        const char *msg = "Can't remove this file.\n";
        write(client_fd, msg, strlen(msg));
    } else {
        LOG_INFO("File removed successfully: %s", filename);
        const char *msg = "File has been removed.\n";
        write(client_fd, msg, strlen(msg));
    }
    write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
}

void handle_pwd(int client_fd) {
    char cwd[BUFSIZ];
    LOG_DEBUG("Attempting to get current working directory.");
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        LOG_ERROR("Failed to get current working directory.");
        perror("Get current working directory failed");
        const char *msg = "Can't get current working directory.\n";
        write(client_fd, msg, strlen(msg));
    } else {
        LOG_INFO("Current working directory: %s", cwd);
        write(client_fd, cwd, strlen(cwd));
        write(client_fd, "\n", 1);
    }
    write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
}

void handle_mkdir(int client_fd, const char *dirname) {
    LOG_DEBUG("Attempting to create directory: %s", dirname);
    if (mkdir(dirname, 0775) == -1) {
        LOG_ERROR("Failed to create directory: %s", dirname);
        perror("Create directory failed");
        const char *msg = "Can't create directory.\n";
        write(client_fd, msg, strlen(msg));
    } else {
        LOG_INFO("Directory created successfully: %s", dirname);
        const char *msg = "Directory created successfully.\n";
        write(client_fd, msg, strlen(msg));
    }
    write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
}
