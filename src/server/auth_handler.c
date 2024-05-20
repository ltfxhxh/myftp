#include "auth_handler.h"
#include "database.h"
#include "logger.h"
#include <json-c/json.h>
#include <openssl/sha.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#define SALT_LENGTH 16
#define HASH_LENGTH 64

void generate_salt(char *salt, size_t length) {
    const char *charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (size_t i = 0; i < length - 1; i++) {
        salt[i] = charset[rand() % (strlen(charset))];
    }
    salt[length - 1] = '\0';
}

void custom_hash_password(const char *password, const char *salt, char *hashed_password) {
    char salted_password[256];
    snprintf(salted_password, sizeof(salted_password), "%s%s", password, salt);

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, salted_password, strlen(salted_password));
    SHA256_Final(hash, &sha256);

    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(hashed_password + (i * 2), "%02x", hash[i]);
    }
    hashed_password[HASH_LENGTH] = '\0';
}

void handle_register_request(int client_fd, const char *request) {
    json_object *parsed_request = json_tokener_parse(request);
    const char *username = json_object_get_string(json_object_object_get(parsed_request, "username"));
    const char *password = json_object_get_string(json_object_object_get(parsed_request, "password"));

    MYSQL *conn = db_connect();
    if (conn == NULL) {
        send(client_fd, "error: database connection failed", strlen("error: database connection failed"), 0);
        json_object_put(parsed_request);
        return;
    }

    if (check_username_exists(conn, username) == 1) {
        LOG_WARNING("Username already exists: %s", username);
        send(client_fd, "error: username already exists", strlen("error: username already exists"), 0);
        db_disconnect(conn);
        json_object_put(parsed_request);
        return;
    }

    char salt[SALT_LENGTH];
    generate_salt(salt, sizeof(salt));

    char hashed_password[HASH_LENGTH + 1];
    custom_hash_password(password, salt, hashed_password);

    if (store_user(conn, username, hashed_password, salt, "/") == 0) {
        LOG_INFO("User registered successfully: %s", username);
        send(client_fd, "success", strlen("success"), 0);
    } else {
        LOG_ERROR("Failed to store user: %s", username);
        send(client_fd, "error: failed to register user", strlen("error: failed to register user"), 0);
    }

    db_disconnect(conn);
    json_object_put(parsed_request);
}

void handle_login_request(int client_fd, const char *request) {
    json_object *parsed_request = json_tokener_parse(request);
    const char *username = json_object_get_string(json_object_object_get(parsed_request, "username"));
    const char *password = json_object_get_string(json_object_object_get(parsed_request, "password"));

    MYSQL *conn = db_connect();
    if (conn == NULL) {
        send(client_fd, "error: database connection failed", strlen("error: database connection failed"), 0);
        json_object_put(parsed_request);
        return;
    }

    char stored_hash[HASH_LENGTH + 1];
    char salt[SALT_LENGTH];
    char pwd[256];

    if (get_user(conn, username, stored_hash, salt, pwd) == 0) {
        char hashed_password[HASH_LENGTH + 1];
        custom_hash_password(password, salt, hashed_password);

        if (strcmp(hashed_password, stored_hash) == 0) {
            LOG_INFO("User logged in successfully: %s", username);
            send(client_fd, "success", strlen("success"), 0);
        } else {
            LOG_WARNING("Incorrect password for user: %s", username);
            send(client_fd, "error: incorrect password", strlen("error: incorrect password"), 0);
        }
    } else {
        LOG_WARNING("User not found: %s", username);
        send(client_fd, "error: user not found", strlen("error: user not found"), 0);
    }

    db_disconnect(conn);
    json_object_put(parsed_request);
}

