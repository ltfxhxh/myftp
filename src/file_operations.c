#include "file_operations.h"
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

const char *END_OF_MESSAGE = "\n.\n";

void handle_cd(int client_fd, const char *path) {
    if (chdir(path) == -1) {
        perror("chdir error");
        const char *msg = "Change Directory Failed.\n";
        write(client_fd, msg, strlen(msg));
    } else {
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

    if ((dir = opendir(".")) == NULL) {
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
        perror("write error");
    }
    write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
    closedir(dir);
}

void handle_puts(int client_fd, const char *filename) {
    int file_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (file_fd < 0) {
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
            perror("Write file error");
            const char *msg = "Puts file error occurred in writing file\n";
            write(client_fd, msg, strlen(msg));
            close(file_fd);
            write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
            return;
        }
    }

    if (readbytes < 0) {
        perror("Read file error");
        const char *msg = "Puts file error occurred in reading file\n";
        write(client_fd, msg, strlen(msg));
    } else {
        const char *msg = "Puts file successfully!\n";
        write(client_fd, msg, strlen(msg));
    }
    write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
    close(file_fd);
}

void handle_gets(int client_fd, const char *filename) {
    int file_fd = open(filename, O_RDONLY);
    if (file_fd < 0) {
        perror("Send file error");
        const char *msg = "Can't send file.\n";
        write(client_fd, msg, strlen(msg));
        write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
        return;
    }

    char buf[BUFSIZ];
    ssize_t readbytes;
    while ((readbytes = read(file_fd, buf, sizeof(buf))) > 0) {
        if (write(client_fd, buf, readbytes) != readbytes) {
            perror("Write file error");
            const char *msg = "Gets file error occurred in writing file\n";
            write(client_fd, msg, strlen(msg));
            close(file_fd);
            write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
            return;
        }
    }

    if (readbytes < 0) {
        perror("Read file error");
        const char *msg = "Gets file error occurred in reading file\n";
        write(client_fd, msg, strlen(msg));
    }
    close(file_fd);
    write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
}

void handle_remove(int client_fd, const char *filename) {
    if (remove(filename) == -1) {
        perror("Remove file failed");
        const char *msg = "Can't remove this file.\n";
        write(client_fd, msg, strlen(msg));
    } else {
        const char *msg = "File has been removed.\n";
        write(client_fd, msg, strlen(msg));
    }
    write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
}

void handle_pwd(int client_fd) {
    char cwd[BUFSIZ];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("Get current working directory failed");
        const char *msg = "Can't get current working directory.\n";
        write(client_fd, msg, strlen(msg));
    } else {
        write(client_fd, cwd, strlen(cwd));
        write(client_fd, "\n", 1);
    }
    write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
}

void handle_mkdir(int client_fd, const char *dirname) {
    if (mkdir(dirname, 0775) == -1) {
        perror("Create directory failed");
        const char *msg = "Can't create directory.\n";
        write(client_fd, msg, strlen(msg));
    } else {
        const char *msg = "Directory created successfully.\n";
        write(client_fd, msg, strlen(msg));
    }
    write(client_fd, END_OF_MESSAGE, strlen(END_OF_MESSAGE));
}

