// login.c
#include "login.h"
#include <openssl/sha.h>

void hash_password(const char *password, char *hashed_password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, password, strlen(password));
    SHA256_Final(hash, &sha256);

    // 转成16进制字符串
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(hashed_password + (i * 2), "%02x", hash[i]);
    }
    hashed_password[64] = 0;
}

void handle_register(int sockfd) {
    char username[BUFFER_SIZE];
    char password[BUFFER_SIZE];
    char password_confirm[BUFFER_SIZE];

    printf("用户名: ");
    fgets(username, BUFFER_SIZE, stdin);
    trim_newline(username);

    printf("密码: ");
    fgets(password, BUFFER_SIZE, stdin);
    trim_newline(password);

    printf("确认密码: ");
    fgets(password_confirm, BUFFER_SIZE, stdin);
    trim_newline(password_confirm);

    // 判断两次密码是否输入一致
    if (strcmp(password, password_confirm) != 0) {
        printf(ANSI_COLOR_RED "哎呀~两次密码不一致: 请重新注册.\n");
        return;
    }

    // 加密
    char hashed_password[BUFFER_SIZE];
    hash_password(password, hashed_password);

    // 构造json格式请求
    char request[BUFFER_SIZE];
    snprintf(request, BUFFER_SIZE, "{\"action\": \"register\", \"username\": \"%s\", \"password\": \"%s\"}", username, hashed_password);

    // 向服务器发送请求
    if (send(sockfd, request, strlen(request), 0) < 0) {
        perror(ANSI_COLOR_RED "Failed to send register request" ANSI_COLOR_RESET);
        return;
    }

    // 等待服务器的回复
    char response[BUFFER_SIZE];
    int bytes_received = recv(sockfd, response, BUFFER_SIZE, 0);
    if (bytes_received < 0) {
        perror(ANSI_COLOR_RED "Failed to receive server response" ANSI_COLOR_RESET);
        return;
    }

    response[bytes_received] = '\0';

    // 处理服务器回复
    if (strcmp(response, "success") == 0) {
        printf(ANSI_COLOR_GREEN "Registration successful!\n" ANSI_COLOR_RESET);
    } else {
        printf(ANSI_COLOR_RED "Registration failed: %s\n" ANSI_COLOR_RESET, response);
    }
}

int handle_login(int sockfd) {
    char username[BUFFER_SIZE];
    char password[BUFFER_SIZE];

    printf("用户名：");
    fgets(username, BUFFER_SIZE, stdin);
    trim_newline(username);

    printf("密码：");
    fgets(password, BUFFER_SIZE, stdin);
    trim_newline(password);

    // 发送前加密密码
    char hashed_password[BUFFER_SIZE];
    hash_password(password, hashed_password);

    // 构造json格式请求
    char request[BUFFER_SIZE];
    snprintf(request, BUFFER_SIZE, "{\"action\": \"login\", \"username\": \"%s\", \"password\": \"%s\"}", username, hashed_password);

    // 发送登录请求
    if (send(sockfd, request, strlen(request), 0) < 0) {
        perror(ANSI_COLOR_RED "Failed to send login request" ANSI_COLOR_RESET);
        return 0;
    }

    // 等待服务器回复
    char response[BUFFER_SIZE];
    int bytes_received = recv(sockfd, response, BUFFER_SIZE, 0);
    if (bytes_received < 0) {
        perror(ANSI_COLOR_RED "Failed to receive server response" ANSI_COLOR_RESET);
        return 0;
    }

    response[bytes_received] = '\0';

    // 处理回复 
    if (strcmp(response, "success") == 0) {
        printf(ANSI_COLOR_GREEN "Login successful!\n" ANSI_COLOR_RESET);
        return 1;
    } else {
        printf(ANSI_COLOR_RED "Login failed: %s\n" ANSI_COLOR_RESET, response);
        return 0;
    }
}
