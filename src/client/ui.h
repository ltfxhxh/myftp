// ui.h
#ifndef UI_H
#define UI_H
#include "common.h"

extern char current_path[1024];

void display_login();
void display_welcome_message();
void trim_newline(char *str);

#endif // UI_H

