#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <time.h>
#include <errno.h>

#define PORT 58053
#define MAX_CLIENTS 10
#define BUFFER_SIZE 256
#define MAX_ATTEMPTS 8
#define MAX_PLAYTIME 600

// Structs
typedef struct {
    int max_playtime;      // Maximum playtime in seconds
    int trials;            // Number of trials made
    int active;            // Active game flag (1: active, 0: inactive)
    time_t start_time;     // Start time of the game
    char secret_key[5];    // Secret key (4 colors + null terminator)
    char plid[7];          // Player ID
    char mode[10];         // store the game mode (PLAY or DEBUG)
    char guesses[8][5];    // Histórico de até 8 tentativas (4 cores + '\0')
} Game;

typedef struct {
    int n_scores;
    char plid[10][7];           // Player ID
    char secret_key[10][5];     // Secret key
    int score[10];              // Game score
    int no_trials[10];          // Number of trials made in the game
    char mode[10][6];               // Game mode (PLAY or DEBUG)
} Scorelist;

// Function prototypes
void initialize_games();
void generate_secret_key(char *secret_key);
int start_new_game(const char *plid, int max_playtime, char *secret_key, char *mode);
int process_guess(const char *plid, const char *guess, int nT, int *nB, int *nW, char *response_buffer);
void quit_game(const char *plid, char *response);
void handle_udp_message(int udp_socket, struct sockaddr_in *client_addr, socklen_t client_len, char *buffer);
void handle_tcp_connection(int client_socket);
void get_trials(const char *plid, char *buffer);
void get_scoreboard();
void create_score_file(const char *plid, const char *code, int trials, const char *mode, int duration, int max_playtime);
int find_last_game(const char *plid, char* fname);
int get_game(const char *plid);
void add_trial(const char *plid, const char *guess, int correct_pos, int wrong_pos, int elapsed_time);
int find_top_scores(Scorelist *list);

#endif