#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <time.h>

#define UDP_PORT 58001
#define TCP_PORT 58002 
#define MAX_CLIENTS 10
#define BUFFER_SIZE 256
#define MAX_ATTEMPTS 8
#define MAX_PLAYTIME 600

// Structs
typedef struct {
    char plid[7];          // Player ID
    char secret_key[5];    // Secret key (4 colors + null terminator)
    int max_playtime;      // Maximum playtime in seconds
    int trials;            // Number of trials made
    int active;            // Active game flag (1: active, 0: inactive)
    char guesses[8][5];    // Histórico de até 8 tentativas (4 cores + '\0')
    time_t start_time;     // Start time of the game
    char mode[10];         // store the game mode (PLAY or DEBUG)
} Game;

typedef struct {
    char plid[7];
    int plays_to_win;
    char secret_key[5];
} Score;

// Function prototypes
void initialize_games();
void generate_secret_key(char *secret_key);
int start_new_game(const char *plid, int max_playtime, char *secret_key);
int process_guess(const char *plid, const char *guess, int nT, int *nB, int *nW, char *response_buffer);
void quit_game(const char *plid, char *response);
void handle_udp_message(int udp_socket, struct sockaddr_in *client_addr, socklen_t client_len, char *buffer);
void handle_tcp_connection(int client_socket);
v