#ifndef REACTOR_H
#define REACTOR_H

#include "epoll_manager.h"

typedef struct Reactor Reactor;

// 创建反应器
Reactor *reactor_create(void);

// 销毁反应器
void reactor_destroy(Reactor *reactor);

// 运行反应器
void reactor_run(Reactor *reactor);

#endif // REACTOR_H

