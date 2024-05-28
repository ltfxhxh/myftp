CC = gcc
CFLAGS = -Wall -g -I/usr/local/include/l8w8jwt
LDFLAGS = -lpthread -lconfig -ljson-c -lmysqlclient -lcrypto -ll8w8jwt # Link with pthread library for thread support

# Source files
SERVER_SRCS = src/server/server.c src/server/epoll_manager.c src/server/thread_pool.c src/server/command_handler.c src/server/file_operations.c src/server/main.c src/server/network_utils.c src/server/config.c src/server/logger.c src/server/auth_handler.c src/server/database.c src/server/jwt_util.c src/server/virtual_file_table.c
CLIENT_SRCS = src/client/client_main.c src/client/file_transfer.c src/client/network.c src/client/ui.c src/client/utils.c src/client/login.c src/client/logger.c
OBJS = $(SERVER_SRCS:.c=.o) $(CLIENT_SRCS:.c=.o)

# Executable targets
SERVER_BIN = bin/server
CLIENT_BIN = bin/client

all: $(SERVER_BIN) $(CLIENT_BIN)

# Server executable
$(SERVER_BIN): $(SERVER_SRCS:.c=.o)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Client executable
$(CLIENT_BIN): $(CLIENT_SRCS:.c=.o)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Generic rule for building objects
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

clean-all: clean
	rm -f $(SERVER_BIN) $(CLIENT_BIN)

.PHONY: all clean clean-all

