#ifndef DATABASE_H
#define DATABASE_H

#include <mysql/mysql.h>

struct VirtualFileRecord {
    int id;
    int parent_id;
    char filename[256];
    int owner_id;
    char md5[33];
    int filesize;
    char type;
    char virtual_path[256];
};

// 数据库连接初始化
MYSQL *db_connect();

// 关闭数据库连接
void db_disconnect(MYSQL *conn);

// 检查用户名是否存在
int check_username_exists(MYSQL *conn, const char *username);

// 存储用户数据
int store_user(MYSQL *conn, const char *username, const char *password_hash, const char *salt, const char *pwd);

// 检索用户数据
int get_user(MYSQL *conn, const char *username, char *password_hash, char *salt, char *pwd, int *user_id);

// 获取虚拟路径对应的记录
int get_virtual_path_record(MYSQL *conn, const char *virtual_path, int owner_id, struct VirtualFileRecord *record);

// 获取用户当前路径
int get_user_current_path(MYSQL *conn, int user_id, char *current_path);

// 更新用户当前路径
int update_user_current_path(MYSQL *conn, int user_id, const char *new_path);

int path_exists_in_vft(MYSQL *conn, int user_id, const char *path);

int insert_directory_into_vft(MYSQL *conn, int user_id, const char *dirname, const char *full_path, int parent_id);

int delete_virtual_file_record(MYSQL *conn, int file_id);

int insert_file_into_vft(MYSQL *conn, int user_id, const char *filename, const char *path);

int get_parent_id(MYSQL *conn, int user_id, const char *current_path);

int directory_exists_in_vft(MYSQL *conn, int user_id, const char *current_path, const char *dirname);

int get_directory_id(MYSQL *conn, int user_id, const char *current_path);

int get_file_id(MYSQL *conn, int user_id, int parent_id, const char *filename);

char get_file_type(MYSQL *conn, int file_id);
#endif // DATABASE_H
