// database.c
#include "database.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int get_user(MYSQL *conn, const char *username, char *password_hash, char *salt, char *pwd) {
    char query[256];
    snprintf(query, sizeof(query), "SELECT salt, hash, pwd FROM users WHERE username='%s'", username);

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
    mysql_free_result(res);

    LOG_INFO("Retrieved user '%s' from database", username);

    return 0;
}
