#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include "database.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <json-c/json.h>

void handle_cd(int client_fd, const char *path, int user_id, MYSQL *conn);
void handle_ls(int client_fd, int user_id, MYSQL *conn);
void handle_mkdir(int client_fd, const char *dirname, int user_id, MYSQL *conn);
void handle_pwd(int client_fd, int user_id, MYSQL *conn);
void handle_remove(int client_fd, const char *filename, int user_id, MYSQL *conn);
void handle_gets(int client_fd, const char *filename, int user_id, MYSQL *conn);
void handle_puts(int client_fd, const char *filename, int user_id, MYSQL *conn);

#endif // FILE_OPERATIONS_H

