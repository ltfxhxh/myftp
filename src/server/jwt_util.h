#ifndef JWT_UTIL_H
#define JWT_UTIL_H

#include <stddef.h>

// 声明 generate_token 函数
char* generate_token(const char *username);

// 声明 verify_token 函数
int verify_token(const char *token);

// 声明 get_user_id_by_username 函数
int get_user_id_by_username(const char *username);

// 声明 extract_user_id_from_token 函数
int extract_user_id_from_token(const char *token);

#endif // JWT_UTIL_H

