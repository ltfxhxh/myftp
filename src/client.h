#ifndef CLIENT_H
#define CLIENT_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Buffer size for reading input and communicating with the server
#define BUFFER_SIZE 1024

// ANSI Color Codes for beautifying the output
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_BOLD_ON       "\x1b[1m"
#define ANSI_BOLD_OFF      "\x1b[22m"

// Function to create a client socket and connect to the server
int create_client_socket(const char *server_ip, int server_port);

// Function to display a welcome message to the user
void display_welcome_message();

// Function to display the available commands to the user
void display_commands();

// Function to process and send the user's command to the server
void process_command(int sockfd, int command);

// Function to interact with the server after connecting
void interact_with_server(int sockfd);

// Function to read a password from the terminal with echo off
void read_password(char *password, size_t max_len);

#endif // CLIENT_H

