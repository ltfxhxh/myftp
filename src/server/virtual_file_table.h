#ifndef VIRTUAL_FILE_TABLE_H
#define VIRTUAL_FILE_TABLE_H

#include <mysql/mysql.h>

// 插入新目录
int insert_directory(MYSQL *conn, const char *name, int parent_id, int owner_id);

// 插入新文件
int insert_file(MYSQL *conn, const char *name, int parent_id, int owner_id, const char *md5, int filesize, const char *actual_path);

// 删除文件或目录
int delete_entry(MYSQL *conn, const char *name, int parent_id, int owner_id, const char *type);

// 获取虚拟路径对应的实际路径
int get_real_path(MYSQL *conn, const char *virtual_path, int owner_id, char *real_path, size_t max_len);

#endif // VIRTUAL_FILE_TABLE_H

