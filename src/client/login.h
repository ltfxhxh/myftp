#ifndef LOGIN_H
#define LOGIN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "common.h"
#include "network.h"
#include "ui.h"
#include "utils.h"

// 注册功能处理函数
void handle_register(int sockfd);

// 登录功能处理函数
int handle_login(int sockfd, char *token, char *usr);

// 辅助函数，例如密码哈希函数
void hash_password(const char *password, char *hashed_password);

#endif // LOGIN_H

