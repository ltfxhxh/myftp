#include "virtual_file_table.h"
#include "database.h"
#include "logger.h"
#include <mysql/mysql.h>
#include <stdlib.h>
#include <string.h>

int insert_directory(MYSQL *conn, const char *name, int parent_id, int owner_id) {
    char query[512];
    snprintf(query, sizeof(query), 
             "INSERT INTO virtual_file_table (filename, parent_id, owner_id, type) VALUES ('%s', %d, %d, 'd')", 
             name, parent_id, owner_id);

    if (mysql_query(conn, query)) {
        LOG_ERROR("INSERT INTO virtual_file_table failed: %s", mysql_error(conn));
        return -1;
    }

    LOG_INFO("Inserted directory '%s' into virtual_file_table", name);
    return 0;
}

int insert_file(MYSQL *conn, const char *name, int parent_id, int owner_id, const char *md5, int filesize, const char *actual_path) {
    char query[512];
    snprintf(query, sizeof(query), 
             "INSERT INTO virtual_file_table (filename, parent_id, owner_id, md5, filesize, type, actual_path) VALUES ('%s', %d, %d, '%s', %d, 'f', '%s')", 
             name, parent_id, owner_id, md5, filesize, actual_path);

    if (mysql_query(conn, query)) {
        LOG_ERROR("INSERT INTO virtual_file_table failed: %s", mysql_error(conn));
        return -1;
    }

    LOG_INFO("Inserted file '%s' into virtual_file_table", name);
    return 0;
}

int delete_entry(MYSQL *conn, const char *name, int parent_id, int owner_id, const char *type) {
    char query[512];
    snprintf(query, sizeof(query), 
             "DELETE FROM virtual_file_table WHERE filename='%s' AND parent_id=%d AND owner_id=%d AND type='%s'", 
             name, parent_id, owner_id, type);

    if (mysql_query(conn, query)) {
        LOG_ERROR("DELETE FROM virtual_file_table failed: %s", mysql_error(conn));
        return -1;
    }

    LOG_INFO("Deleted entry '%s' from virtual_file_table", name);
    return 0;
}

int get_real_path(MYSQL *conn, const char *virtual_path, int owner_id, char *real_path, size_t max_len) {
    char query[512];
    snprintf(query, sizeof(query), 
             "SELECT actual_path FROM virtual_file_table WHERE filename='%s' AND owner_id=%d", 
             virtual_path, owner_id);

    if (mysql_query(conn, query)) {
        LOG_ERROR("SELECT actual_path FROM virtual_file_table failed: %s", mysql_error(conn));
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
        LOG_WARNING("No entry found for virtual_path '%s'", virtual_path);
        return -1;
    }

    strncpy(real_path, row[0], max_len);
    real_path[max_len - 1] = '\0';

    mysql_free_result(res);
    LOG_INFO("Retrieved real path '%s' for virtual_path '%s'", real_path, virtual_path);
    return 0;
}

