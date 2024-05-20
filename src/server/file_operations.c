#include "file_operations.h"
#include "logger.h"  // Include the logger header
#include "network_utils.h"
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

const char *END_OF_MESSAGE = "\n.\n";

void handle_cd(int client_fd, const char *path) {
    LOG_DEBUG("Attempting to change directory to: %s", path);
    if (chdir(path) == -1) {
        LOG_ERROR("Failed to change directory to: %s", path);
        perror("chdir error");
        const char *msg = "Change Directory Failed.\n";
        write(client_fd, msg, strlen(msg));
    } else {
        LOG_INFO("Changed directory to: %s", path);
        const char *msg = "Change Directory Successfully.\n";
        write(client_fd, msg, strlen(msg));
    }
    write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
}

void handle_ls(int client_fd) {
    DIR *dir;
    struct dirent *entry;
    char buffer[1024];
    int buflen = 0;

    LOG_DEBUG("Listing directory contents.");
    if ((dir = opendir(".")) == NULL) {
        LOG_ERROR("Failed to open directory.");
        perror("opendir error");
        const char *msg = "Error: failed to open directory.\n";
        write(client_fd, msg, strlen(msg));
        write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
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
        LOG_ERROR("Failed to open file for writing: %s", filename);
        perror("Puts file error");
        const char *msg = "Can't puts file.\n";
        write(client_fd, msg, strlen(msg));
        write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
        return;
    }

    char buf[BUFSIZ];
    ssize_t readbytes;
    while ((readbytes = read(client_fd, buf, sizeof(buf))) > 0) {
        if (write(file_fd, buf, readbytes) != readbytes) {
            LOG_ERROR("Failed to write to file: %s", filename);
            perror("Write file error");
            const char *msg = "Puts file error occurred in writing file\n";
            write(client_fd, msg, strlen(msg));
            close(file_fd);
            write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
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

void handle_gets(int client_fd, const char *filename, off_t offset) {
    LOG_DEBUG("Attempting to send file: %s", filename);
    int file_fd = open(filename, O_RDONLY);
    if (file_fd < 0) {
        LOG_ERROR("Failed to open file for reading: %s", filename);
        perror("Send file error");
        const char *msg = "Can't send file.\n";
        write(client_fd, msg, strlen(msg));
        write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
        return;
    }

    if (lseek(file_fd, offset, SEEK_SET) == (off_t)-1) {
        LOG_ERROR("Failed to seek in file: %s", filename);
        perror("lseek error");
        close(file_fd);
        const char *msg = "Can't seek in file.\n";
        write(client_fd, msg, strlen(msg));
        write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
        return;
    }

    char buf[BUFSIZ];
    ssize_t readbytes;
    while ((readbytes = read(file_fd, buf, sizeof(buf))) > 0) {
        if (write(client_fd, buf, readbytes) != readbytes) {
            LOG_ERROR("Failed to write to client during GETS operation.");
            perror("Write file error");
            const char *msg = "Gets file error occurred in writing file\n";
            write(client_fd, msg, strlen(msg));
            close(file_fd);
            write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
            return;
        }
    }

    if (readbytes < 0) {
        LOG_ERROR("Error reading file during GETS operation: %s", filename);
        perror("Read file error");
        const char *msg = "Gets file error occurred in reading file\n";
        write(client_fd, msg, strlen(msg));
    } else {
        LOG_INFO("File sent successfully: %s", filename);
    }
    close(file_fd);
    write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
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
