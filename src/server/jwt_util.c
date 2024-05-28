#include "jwt_util.h"
#include "logger.h"
#include "database.h"
#include "config.h"
#include <l8w8jwt/encode.h>
#include <l8w8jwt/decode.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mysql/mysql.h>
#include <errno.h>

#define TOKEN_EXPIRATION_TIME 3600  // Token 有效期为 1 小时


char* generate_token(const char *username) {
    const char *SECRET_KEY = "a8Fj93kLd2NcH4sKv7MzX8p9Tf4Vr1Qb";  // 硬编码的密钥

    char* jwt = NULL; 
    size_t jwt_length = 0; 

    struct l8w8jwt_encoding_params params;
    l8w8jwt_encoding_params_init(&params);

    params.alg = L8W8JWT_ALG_HS512;  
    params.sub = (char *)username;  
    params.iss = "issuer";  
    params.aud = "audience";  
    params.exp = time(NULL) + TOKEN_EXPIRATION_TIME;  
    params.iat = time(NULL);  
 
    params.secret_key = (unsigned char*)SECRET_KEY;
    params.secret_key_length = strlen(SECRET_KEY);
    params.out = &jwt;
    params.out_length = &jwt_length;

    int result = l8w8jwt_encode(&params);
    if (result != L8W8JWT_SUCCESS) {
        LOG_ERROR("生成 Token 失败: %d", result);
        return NULL;
    }

    LOG_DEBUG("token: %s", jwt);
    return jwt;
}

int verify_token(const char *token) {
    const char *SECRET_KEY = "a8Fj93kLd2NcH4sKv7MzX8p9Tf4Vr1Qb";  // 硬编码的密钥
    struct l8w8jwt_decoding_params params;
    l8w8jwt_decoding_params_init(&params);

    params.alg = L8W8JWT_ALG_HS512;  
    params.jwt = (char*)token;
    params.jwt_length = strlen(token);
    params.verification_key = (unsigned char*)SECRET_KEY;
    params.verification_key_length = strlen(SECRET_KEY);

    struct l8w8jwt_claim *claims = NULL;
    size_t claim_count = 0;
    enum l8w8jwt_validation_result validation_result;

    int decode_result = l8w8jwt_decode(&params, &validation_result, &claims, &claim_count);
    if (decode_result == L8W8JWT_SUCCESS && validation_result == L8W8JWT_VALID) {
        time_t now = time(NULL);
        for (size_t i = 0; i < claim_count; i++) {
            if (strcmp(claims[i].key, "exp") == 0) {
                if (now > atoi(claims[i].value)) {
                    LOG_ERROR("Token 已过期");
                    l8w8jwt_free_claims(claims, claim_count);
                    return 0;  // Token 已过期
                }
            }
        }
        l8w8jwt_free_claims(claims, claim_count);
        return 1;  // 验证成功
    } else {
        LOG_ERROR("验证 Token 失败: %d", decode_result);
        return 0;  // 验证失败
    }
}

int get_user_id_by_username(const char *username) {
    MYSQL *conn = db_connect();
    if (conn == NULL) {
        LOG_ERROR("数据库连接失败");
        return -1;
    }

    char query[256];
    snprintf(query, sizeof(query), "SELECT id FROM users WHERE username='%s'", username);

    if (mysql_query(conn, query)) {
        LOG_ERROR("查询用户ID失败: %s", mysql_error(conn));
        db_disconnect(conn);
        return -1;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (res == NULL) {
        LOG_ERROR("存储查询结果失败: %s", mysql_error(conn));
        db_disconnect(conn);
        return -1;
    }

    MYSQL_ROW row = mysql_fetch_row(res);
    int user_id = -1;
    if (row) {
        user_id = atoi(row[0]);
    }

    mysql_free_result(res);
    db_disconnect(conn);
    return user_id;
}

int extract_user_id_from_token(const char *token) {
    const char *SECRET_KEY = "a8Fj93kLd2NcH4sKv7MzX8p9Tf4Vr1Qb";  // 硬编码的密钥

    struct l8w8jwt_decoding_params params;
    l8w8jwt_decoding_params_init(&params);

    params.alg = L8W8JWT_ALG_HS512;  
    params.jwt = (char*)token;
    params.jwt_length = strlen(token);
    params.verification_key = (unsigned char*)SECRET_KEY;
    params.verification_key_length = strlen(SECRET_KEY);

    struct l8w8jwt_claim *claims = NULL;
    size_t claim_count = 0;
    enum l8w8jwt_validation_result validation_result;

    int decode_result = l8w8jwt_decode(&params, &validation_result, &claims, &claim_count);
    if (decode_result != L8W8JWT_SUCCESS || validation_result != L8W8JWT_VALID) {
        LOG_ERROR("解码 Token 失败: %d", decode_result);
        return -1;  // 解码失败
    }

    const char *username = NULL;
    for (size_t i = 0; i < claim_count; i++) {
        if (strcmp(claims[i].key, "sub") == 0) {  // 确保使用正确的键 "sub"
            username = claims[i].value;
            break;
        }
    }

    if (username == NULL) {
        LOG_ERROR("从 Token 中获取用户名失败");
        l8w8jwt_free_claims(claims, claim_count);
        return -1;  // 无法获取用户名
    }

    LOG_DEBUG("从 Token 中提取的用户名: %s", username);
    int user_id = get_user_id_by_username(username);
    l8w8jwt_free_claims(claims, claim_count);

    if (user_id == -1) {
        LOG_ERROR("获取用户 ID 失败");
    }

    return user_id;
}

