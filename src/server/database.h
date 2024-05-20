#ifndef DATABASE_H
#define DATABASE_H

#include <mysql/mysql.h>

// 数据库连接初始化
MYSQL *db_connect();

// 关闭数据库连接
void db_disconnect(MYSQL *conn);

// 检查用户名是否存在
int check_username_exists(MYSQL *conn, const char *username);

// 存储数据
int store_user(MYSQL *conn, const char *username, const char *password_hash, const char *salt, const char *pwd);

// 检索用户数据
int get_user(MYSQL *conn, const char *username, char *password_hash, char *salt, char *pwd);

#endif
