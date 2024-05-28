#ifndef AUTH_HANDLER_H
#define AUTH_HANDLER_H

#include <json-c/json.h>

// 处理注册请求
void handle_register_request(int client_fd, json_object *args);

// 处理登录请求
void handle_login_request(int client_fd, json_object *args);

// 验证 token
int validate_token(const char *token);

// 从 token 中获取 user_id
int get_user_id_from_token(const char *token);

#endif // AUTH_HANDLER_H

