#include "login.h"
#include <openssl/sha.h>
#include <json-c/json.h>
#include <termios.h>

void get_password(char *password, size_t size) {
    struct termios oldt, newt;
    int ch, i = 0;

    // 获取当前终端设置并保存
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;

    // 禁用回显
    newt.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    // 逐字符读取密码输入
    while ((ch = getchar()) != '\n' && ch != EOF && i < size - 1) {
        password[i++] = ch;
        putchar('*'); // 显示替代字符
    }
    password[i] = '\0';

    // 恢复终端设置
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    putchar('\n');
}

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
    struct json_object *json_request = json_object_new_object();
    json_object_object_add(json_request, "command", json_object_new_string("register"));
    struct json_object *args = json_object_new_object();
    json_object_object_add(args, "username", json_object_new_string(username));
    json_object_object_add(args, "password", json_object_new_string(hashed_password));
    json_object_object_add(json_request, "args", args);
    const char *request = json_object_to_json_string(json_request);

    // 向服务器发送请求
    if (send(sockfd, request, strlen(request), 0) < 0) {
        perror(ANSI_COLOR_RED "Failed to send register request" ANSI_COLOR_RESET);
        json_object_put(json_request); // 释放 JSON 对象
        return;
    }

    // 等待服务器的回复
    char response[BUFFER_SIZE];
    int bytes_received = recv(sockfd, response, BUFFER_SIZE, 0);
    if (bytes_received < 0) {
        perror(ANSI_COLOR_RED "Failed to receive server response" ANSI_COLOR_RESET);
        json_object_put(json_request); // 释放 JSON 对象
        return;
    }

    response[bytes_received] = '\0';

    // 处理服务器回复
    struct json_object *parsed_response = json_tokener_parse(response);
    int success = json_object_get_boolean(json_object_object_get(parsed_response, "success"));
    if (success) {
        printf(ANSI_COLOR_GREEN "Registration successful!\n" ANSI_COLOR_RESET);
    } else {
        const char *message = json_object_get_string(json_object_object_get(parsed_response, "message"));
        printf(ANSI_COLOR_RED "Registration failed: %s\n" ANSI_COLOR_RESET, message);
    }

    json_object_put(parsed_response); // 释放 JSON 对象
    json_object_put(json_request); // 释放 JSON 对象
}

int handle_login(int sockfd, char *token, char *usr) {
    char username[BUFFER_SIZE];
    char password[BUFFER_SIZE];

    printf("用户名：");
    fgets(username, BUFFER_SIZE, stdin);
    trim_newline(username);

    printf("密码：");
     get_password(password, BUFFER_SIZE); // 使用自定义的密码输入函数

    // 发送前加密密码
    char hashed_password[BUFFER_SIZE];
    hash_password(password, hashed_password);

    // 构造json格式请求
    struct json_object *json_request = json_object_new_object();
    json_object_object_add(json_request, "command", json_object_new_string("login"));
    struct json_object *args = json_object_new_object();
    json_object_object_add(args, "username", json_object_new_string(username));
    json_object_object_add(args, "password", json_object_new_string(hashed_password));
    json_object_object_add(json_request, "args", args);
    const char *request = json_object_to_json_string(json_request);

    // 发送登录请求
    if (send(sockfd, request, strlen(request), 0) < 0) {
        perror(ANSI_COLOR_RED "Failed to send login request" ANSI_COLOR_RESET);
        json_object_put(json_request); // 释放 JSON 对象
        return 0;
    }

    // 等待服务器回复
    char response[BUFFER_SIZE];
    int bytes_received = recv(sockfd, response, BUFFER_SIZE, 0);
    if (bytes_received < 0) {
        perror(ANSI_COLOR_RED "Failed to receive server response" ANSI_COLOR_RESET);
        json_object_put(json_request); // 释放 JSON 对象
        return 0;
    }

    response[bytes_received] = '\0';

    // 处理回复 
    struct json_object *parsed_response = json_tokener_parse(response);
    int success = json_object_get_boolean(json_object_object_get(parsed_response, "success"));
    if (success) {
        printf(ANSI_COLOR_GREEN "Login successful!\n" ANSI_COLOR_RESET);
        const char *received_token = json_object_get_string(json_object_object_get(parsed_response, "token"));
        strncpy(token, received_token, BUFFER_SIZE);
        strncpy(usr, username, BUFFER_SIZE);
        token[BUFFER_SIZE - 1] = '\0'; // 确保token字符串以空字符结尾
        json_object_put(parsed_response); // 释放 JSON 对象
        json_object_put(json_request); // 释放 JSON 对象
        return 1;
    } else {
        const char *message = json_object_get_string(json_object_object_get(parsed_response, "message"));
        printf(ANSI_COLOR_RED "Login failed: %s\n" ANSI_COLOR_RESET, message);
        json_object_put(parsed_response); // 释放 JSON 对象
        json_object_put(json_request); // 释放 JSON 对象
        return 0;
    }
}

