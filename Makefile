CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lpthread  # Link with pthread library for thread support

# Source files
SERVER_SRCS = src/server.c src/epoll_manager.c src/thread_pool.c src/command_handler.c src/file_operations.c src/main.c src/network_utils.c
CLIENT_SRCS = src/client.c
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
	rm -f $(OBJS) $(SERVER_BIN) $(CLIENT_BIN)

.PHONY: all clean

