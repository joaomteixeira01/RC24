#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <time.h>

#define UDP_PORT "58001"
#define TCP_PORT "58002" 
#define MAX_CLIENTS 10
#define BUFFER_SIZE 256

// Structs
typedef struct {
    char plid[7];          // Player ID
    char secret_key[5];    // Secret key (4 colors + null terminator)
    int max_playtime;      // Maximum playtime in seconds
    int trials;            // Number of trials made
    int active;            // Active game flag (1: active, 0: inactive)
} Game;

typedef struct {
    char plid[7];
    int plays_to_win;
    char secret_key[5];
} Score;

// Functions
//void start_udp_server();
//void handle_udp_message(int udp_socket, struct sockaddr_in *client_addr, socklen_t client_len, char *buffer);
//void start_tcp_server();
//void handle_tcp_connection(int tcp_socket);

// Prototypes for UDP server
void start_udp_server();
void handle_udp_message(int udp_socket, struct sockaddr_in *client_addr, socklen_t client_len, char *buffer);

// Prototypes for TCP server
void start_tcp_server();
void handle_tcp_connection(int tcp_socket);

// Game logic functions
int start_new_game(const char *plid, int max_playtime, char *secret_key);
int process_guess(const char *plid, const char *guess, int *nB, int *nW);
void quit_game(const char *plid, char *response);
void generate_secret_key(char *secret_key);

// Utility for file-based responses
void get_trials(const char *plid, char *buffer);
void get_scoreboard(char *buffer);

#endif