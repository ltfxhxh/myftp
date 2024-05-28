#include "database.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>

MYSQL *db_connect() {
    MYSQL *conn = mysql_init(NULL);
    if (conn == NULL) {
        LOG_ERROR("mysql_init() failed");
        return NULL;
    }

    if (mysql_real_connect(conn, "localhost", "myftpuser", "125521Xhxh.", "myftp", 0, NULL, 0) == NULL) {
        LOG_ERROR("mysql_real_connect() failed: %s", mysql_error(conn));
        mysql_close(conn);
        return NULL;
    }

    LOG_INFO("Connected to database successfully");
    return conn;
}

void db_disconnect(MYSQL *conn) {
    if (conn != NULL) {
        mysql_close(conn);
        LOG_INFO("Disconnected from database");
    }
}

int check_username_exists(MYSQL *conn, const char *username) {
    char query[256];
    snprintf(query, sizeof(query), "SELECT COUNT(*) FROM users WHERE username='%s'", username);

    if (mysql_query(conn, query)) {
        LOG_ERROR("SELECT COUNT(*) failed: %s", mysql_error(conn));
        return -1;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (res == NULL) {
        LOG_ERROR("mysql_store_result() failed: %s", mysql_error(conn));
        return -1;
    }

    MYSQL_ROW row = mysql_fetch_row(res);
    int count = atoi(row[0]);
    mysql_free_result(res);

    LOG_INFO("Checked username '%s': %d", username, count);
    return count > 0 ? 1 : 0;
}

int store_user(MYSQL *conn, const char *username, const char *password_hash, const char *salt, const char *pwd) {
    char query[512];
    snprintf(query, sizeof(query), 
             "INSERT INTO users (username, salt, hash, pwd) VALUES ('%s', '%s', '%s', '%s')", 
             username, salt, password_hash, pwd);

    if (mysql_query(conn, query)) {
        LOG_ERROR("INSERT INTO users failed: %s", mysql_error(conn));
        return -1;
    }

    LOG_INFO("Stored user '%s' in database", username);
    return 0;
}

int get_user(MYSQL *conn, const char *username, char *password_hash, char *salt, char *pwd, int *user_id) {
    char query[256];
    snprintf(query, sizeof(query), "SELECT salt, hash, pwd, id FROM users WHERE username='%s'", username);

    if (mysql_query(conn, query)) {
        LOG_ERROR("SELECT FROM users failed: %s", mysql_error(conn));
        return -1;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (res == NULL) {
        LOG_ERROR("mysql_store_result() failed: %s", mysql_error(conn));
        return -1;
    }

    MYSQL_ROW row = mysql_fetch_row(res);
    if (row == NULL) {
        mysql_free_result(res);
        LOG_WARNING("No user found with username '%s'", username);
        return -1;
    }

    strcpy(salt, row[0]);
    strcpy(password_hash, row[1]);
    strcpy(pwd, row[2]);
    *user_id = atoi(row[3]);
    mysql_free_result(res);

    LOG_INFO("Retrieved user '%s' from database", username);
    return 0;
}

int get_virtual_path_record(MYSQL *conn, const char *virtual_path, int owner_id, struct VirtualFileRecord *record) {
    char query[512];
    snprintf(query, sizeof(query), "SELECT id, parent_id, filename, owner_id, md5, filesize, type, virtual_path FROM virtual_file_table WHERE virtual_path='%s' AND owner_id=%d", virtual_path, owner_id);

    if (mysql_query(conn, query)) {
        LOG_ERROR("SELECT FROM virtual_file_table failed: %s", mysql_error(conn));
        return -1;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (res == NULL) {
        LOG_ERROR("mysql_store_result() failed: %s", mysql_error(conn));
        return -1;
    }

    MYSQL_ROW row = mysql_fetch_row(res);
    if (row == NULL) {
        mysql_free_result(res);
        LOG_WARNING("No record found for virtual_path='%s' and owner_id=%d", virtual_path, owner_id);
        return -1;
    }

    record->id = atoi(row[0]);
    record->parent_id = atoi(row[1]);
    strcpy(record->filename, row[2]);
    record->owner_id = atoi(row[3]);
    strcpy(record->md5, row[4]);
    record->filesize = atoi(row[5]);
    record->type = row[6][0];
    strcpy(record->virtual_path, row[7]);

    mysql_free_result(res);
    LOG_INFO("Retrieved record for virtual_path='%s' and owner_id=%d", virtual_path, owner_id);
    return 0;
}


int get_user_current_path(MYSQL *conn, int user_id, char *current_path) {
    char query[256];
    snprintf(query, sizeof(query), "SELECT pwd FROM users WHERE id=%d", user_id);

    if (mysql_query(conn, query)) {
        LOG_ERROR("SELECT pwd FROM users failed: %s", mysql_error(conn));
        return -1;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (res == NULL) {
        LOG_ERROR("mysql_store_result() failed: %s", mysql_error(conn));
        return -1;
    }

    MYSQL_ROW row = mysql_fetch_row(res);
    if (row == NULL) {
        mysql_free_result(res);
        LOG_WARNING("No user found with id=%d", user_id);
        return -1;
    }

    strcpy(current_path, row[0]);
    mysql_free_result(res);

    LOG_INFO("Retrieved current path for user_id=%d: %s", user_id, current_path);
    return 0;
}

int update_user_current_path(MYSQL *conn, int user_id, const char *new_path) {
    char query[256];
    snprintf(query, sizeof(query), "UPDATE users SET pwd='%s' WHERE id=%d", new_path, user_id);

    if (mysql_query(conn, query)) {
        LOG_ERROR("UPDATE users SET pwd failed: %s", mysql_error(conn));
        return -1;
    }

    LOG_INFO("Updated current path for user_id=%d to %s", user_id, new_path);
    return 0;
}

int path_exists_in_vft(MYSQL *conn, int user_id, const char *path) {
    char query[512];
    snprintf(query, sizeof(query), "SELECT COUNT(*) FROM virtual_file_table WHERE owner_id = %d AND virtual_path = '%s'", user_id, path);

    if (mysql_query(conn, query)) {
        LOG_ERROR("path_exists_in_vft: %s", mysql_error(conn));
        return 0;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (res == NULL) {
        LOG_ERROR("path_exists_in_vft: %s", mysql_error(conn));
        return 0;
    }

    MYSQL_ROW row = mysql_fetch_row(res);
    int exists = (row != NULL && atoi(row[0]) > 0);

    mysql_free_result(res);
    return exists;
}


int insert_directory_into_vft(MYSQL *conn, int user_id, const char *dirname, const char *full_path, int parent_id) {
    char query[512];
    snprintf(query, sizeof(query), "INSERT INTO virtual_file_table (owner_id, filename, virtual_path, type, parent_id) VALUES (%d, '%s', '%s', 'd', %d)", user_id, dirname, full_path, parent_id);

    if (mysql_query(conn, query)) {
        LOG_ERROR("INSERT INTO virtual_file_table failed: %s", mysql_error(conn));
        return -1;
    }

    return 0;
}

int delete_virtual_file_record(MYSQL *conn, int file_id) {
    char query[256];
    snprintf(query, sizeof(query), "DELETE FROM virtual_file_table WHERE id = %d", file_id);

    if (mysql_query(conn, query)) {
        LOG_ERROR("DELETE FROM virtual_file_table failed: %s", mysql_error(conn));
        return -1;
    }

    LOG_INFO("Deleted file record with id=%d from virtual_file_table", file_id);
    return 0;
}


int insert_file_into_vft(MYSQL *conn, int user_id, const char *filename, const char *path) {
    if (conn == NULL || filename == NULL || path == NULL) {
        LOG_ERROR("Invalid arguments to insert_file_into_vft");
        return -1;
    }

    // 构建完整路径
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s", path, filename);

    // 插入虚拟文件表的 SQL 查询
    char query[1024];
    snprintf(query, sizeof(query),
             "INSERT INTO virtual_file_table (user_id, filename, virtual_path) VALUES (%d, '%s', '%s')",
             user_id, filename, full_path);

    // 执行 SQL 查询
    if (mysql_query(conn, query)) {
        LOG_ERROR("Failed to insert file into virtual file table: %s", mysql_error(conn));
        return -1;
    }

    LOG_INFO("Inserted file into virtual file table: user_id=%d, filename=%s, path=%s", user_id, filename, full_path);
    return 0;
}

int get_parent_id(MYSQL *conn, int user_id, const char *current_path) {
    // 根目录的父目录ID是0
    if (strcmp(current_path, "/") == 0) {
        return 0;
    }

    // 查询当前路径的父目录ID
    char query[512];
    snprintf(query, sizeof(query), "SELECT id FROM virtual_file_table WHERE owner_id=%d AND virtual_path='%s'", user_id, current_path);

    if (mysql_query(conn, query)) {
        LOG_ERROR("SELECT id FROM virtual_file_table failed: %s", mysql_error(conn));
        return -1;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (res == NULL) {
        LOG_ERROR("mysql_store_result() failed: %s", mysql_error(conn));
        return -1;
    }

    MYSQL_ROW row = mysql_fetch_row(res);
    int parent_id = -1;
    if (row != NULL) {
        parent_id = atoi(row[0]);
    }

    mysql_free_result(res);
    return parent_id;
}

// 检查当前路径下是否存在指定目录
int directory_exists_in_vft(MYSQL *conn, int user_id, const char *current_path, const char *dirname) {
    char query[512];
    snprintf(query, sizeof(query),
             "SELECT COUNT(*) FROM virtual_file_table WHERE owner_id = %d AND virtual_path = '%s' AND filename = '%s' AND type = 'd'",
             user_id, current_path, dirname);

    if (mysql_query(conn, query)) {
        LOG_ERROR("SELECT COUNT(*) FROM virtual_file_table failed: %s", mysql_error(conn));
        return 0;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (res == NULL) {
        LOG_ERROR("mysql_store_result() failed: %s", mysql_error(conn));
        return 0;
    }

    MYSQL_ROW row = mysql_fetch_row(res);
    int exists = atoi(row[0]) > 0;

    mysql_free_result(res);
    return exists;
}

int get_directory_id(MYSQL *conn, int user_id, const char *current_path) {
    if (strcmp(current_path, "/") == 0) {
        return 0;
    }
    char query[512];
    snprintf(query, sizeof(query),
             "SELECT id FROM virtual_file_table WHERE owner_id = %d AND virtual_path = '%s' AND type = 'd'",
             user_id, current_path);

    if (mysql_query(conn, query)) {
        LOG_ERROR("SELECT id FROM virtual_file_table failed: %s", mysql_error(conn));
        return -1;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (res == NULL) {
        LOG_ERROR("mysql_store_result() failed: %s", mysql_error(conn));
        return -1;
    }

    MYSQL_ROW row = mysql_fetch_row(res);
    int id = -1;
    if (row != NULL) {
        id = atoi(row[0]);
    }

    mysql_free_result(res);
    return id;
}

int get_file_id(MYSQL *conn, int user_id, int parent_id, const char *filename) {
    char query[512];
    snprintf(query, sizeof(query), "SELECT id FROM virtual_file_table WHERE owner_id = %d AND parent_id = %d AND filename = '%s'", user_id, parent_id, filename);

    if (mysql_query(conn, query)) {
        LOG_ERROR("SELECT id FROM virtual_file_table failed: %s", mysql_error(conn));
        return -1;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (res == NULL) {
        LOG_ERROR("mysql_store_result() failed: %s", mysql_error(conn));
        return -1;
    }

    MYSQL_ROW row = mysql_fetch_row(res);
    int file_id = -1;
    if (row != NULL) {
        file_id = atoi(row[0]);
    }

    mysql_free_result(res);
    return file_id;
}

char get_file_type(MYSQL *conn, int file_id) {
    char query[256];
    snprintf(query, sizeof(query), "SELECT type FROM virtual_file_table WHERE id=%d", file_id);

    if (mysql_query(conn, query)) {
        LOG_ERROR("SELECT type failed: %s", mysql_error(conn));
        return '\0'; // 出错返回空字符
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (res == NULL) {
        LOG_ERROR("mysql_store_result() failed: %s", mysql_error(conn));
        return '\0';
    }

    MYSQL_ROW row = mysql_fetch_row(res);
    char file_type = row ? row[0][0] : '\0';

    mysql_free_result(res);

    return file_type;
}

