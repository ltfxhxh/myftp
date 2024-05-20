#ifndef AUTH_HANDLER_H
#define AUTH_HANDLER_H

void handle_register_request(int client_fd, const char *request);
void handle_login_request(int client_fd, const char *request);

#endif // AUTH_HANDLER_H

