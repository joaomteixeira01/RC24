/*
 * server.c
 * 
 * This is a combined server for handling both UDP and TCP connections in a simple Mastermind-style game. 
 * It uses `select` to manage multiple clients at the same time without blocking.
 * 
 * What it does:
 * - Handles UDP commands like starting a game (SNG), making guesses (TRY), and quitting (QUT).
 * - Handles TCP requests for things like getting trial summaries (STR) and the scoreboard (SSB).
 * - Keeps track of active games, generates secret keys, and manages game state for multiple players.
 * 
 * The server supports up to MAX_CLIENTS and responds to each client based on their requests.
 */


#include "server.h"
#include <string.h>

// Array to store active games for all connected clients
Game active_games[MAX_CLIENTS];

int main() {
    int udp_socket, tcp_socket, max_fd;
    struct sockaddr_in udp_addr, tcp_addr, client_addr;
    fd_set read_fds;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len;

    // Initialize the game state
    initialize_games();

    // Create UDP socket
    if ((udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("UDP socket");
        exit(EXIT_FAILURE);
    }

    // Configure the UDP socket address
    memset(&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;          // IPv4
    udp_addr.sin_addr.s_addr = INADDR_ANY;  // Accept connections from any address
    udp_addr.sin_port = htons(UDP_PORT);    // Set the UDP port

    // Bind the UDP socket to the specified address
    if (bind(udp_socket, (struct sockaddr*)&udp_addr, sizeof(udp_addr)) < 0) {
        perror("UDP bind");
        close(udp_socket);
        exit(EXIT_FAILURE);
    }

    // Create TCP socket
    if ((tcp_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("TCP socket");
        exit(EXIT_FAILURE);
    }

    // Configure the TCP socket address
    memset(&tcp_addr, 0, sizeof(tcp_addr));
    tcp_addr.sin_family = AF_INET;          // IPv4
    tcp_addr.sin_addr.s_addr = INADDR_ANY;  // Accept connections from any address
    tcp_addr.sin_port = htons(TCP_PORT);    // Set the TCP port

    // Bind the TCP socket to the specified address
    if (bind(tcp_socket, (struct sockaddr*)&tcp_addr, sizeof(tcp_addr)) < 0) {
        perror("TCP bind");
        close(udp_socket);
        close(tcp_socket);
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming TCP connections
    if (listen(tcp_socket, MAX_CLIENTS) < 0) {
        perror("TCP listen");
        close(udp_socket);
        close(tcp_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server running on UDP port %d and TCP port %d\n", UDP_PORT, TCP_PORT);

    // Main server loop: Use select to handle multiple sockets
    while (1) {
        // Clear the file descriptor set and add the sockets
        FD_ZERO(&read_fds);
        FD_SET(udp_socket, &read_fds);
        FD_SET(tcp_socket, &read_fds);

        // Determine the highest file descriptor value
        max_fd = udp_socket > tcp_socket ? udp_socket : tcp_socket;

        // Use select to wait for activity on the sockets
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select");
            break;
        }

        // If the UDP socket is ready, handle an incoming message
        if (FD_ISSET(udp_socket, &read_fds)) {
            addr_len = sizeof(client_addr);
            memset(buffer, 0, BUFFER_SIZE);
            //recvfrom(udp_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &addr_len);
            //printf("Received UDP message: %s\n", buffer);
            //handle_udp_message(udp_socket, &client_addr, addr_len, buffer);

            printf("Waiting for UDP message...\n"); // Added Debug

            ssize_t n = recvfrom(udp_socket, buffer, BUFFER_SIZE, 0, 
                                (struct sockaddr*)&client_addr, &addr_len);
            if (n < 0) {
                perror("recvfrom failed");
            } else {
                buffer[n] = '\0'; // Null-terminate received data
                printf("Received UDP message: %s", buffer);  // Confirm reception
            }

            handle_udp_message(udp_socket, &client_addr, addr_len, buffer);
        }

        // If the TCP socket is ready, handle a new client connection
        if (FD_ISSET(tcp_socket, &read_fds)) {
            int client_socket = accept(tcp_socket, (struct sockaddr*)&client_addr, &addr_len);
            if (client_socket < 0) {
                perror("TCP accept");
                continue;   // Continue handling other connections
            }
            printf("New TCP client connected\n");
            handle_tcp_connection(client_socket);
            close(client_socket);   // Close the client socket after handling the request
        }
    }
    
    // Close both sockets before exiting
    close(udp_socket);
    close(tcp_socket);
    return 0;
}

// Initialize the active games array
void initialize_games() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        active_games[i].active = 0; // Mark all games as inactive
    }
}

// Generate a random secret key for the game
void generate_secret_key(char *secret_key) {
    const char colors[] = "RGBYOP";
    for (int i = 0; i < 4; i++) {
        secret_key[i] = colors[rand() % 6];
    }
    secret_key[4] = '\0';   // Null-terminate the string
}

// Start a new game for the given player
int start_new_game(const char *plid, int max_playtime, char *secret_key) {
    // Check if the player already has an active game
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (active_games[i].active && strcmp(active_games[i].plid, plid) == 0) {
            return 0; // Player already has an active game
        }
    }

    // Find an empty slot to create a new game
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!active_games[i].active) {
            strcpy(active_games[i].plid, plid);
            active_games[i].max_playtime = max_playtime;
            active_games[i].trials = 0;
            active_games[i].active = 1;
            generate_secret_key(active_games[i].secret_key);
            strcpy(secret_key, active_games[i].secret_key);
            return 1;
        }
    }
    return 0; // No available slots
}

// Process a player's guess and update the game state
int process_guess(const char *plid, const char *guess, int *nB, int *nW) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (active_games[i].active && strcmp(active_games[i].plid, plid) == 0) {
            *nB = *nW = 0;

            // Compare the guess to the secret key
            for (int j = 0; j < 4; j++) {
                if (guess[j] == active_games[i].secret_key[j]) {
                    (*nB)++;
                } else if (strchr(active_games[i].secret_key, guess[j])) {
                    (*nW)++;
                }
            }
            active_games[i].trials++;

            // End the game if guessed correctly or max trials reached
            if (*nB == 4 || active_games[i].trials >= 8) {
                active_games[i].active = 0; // End the game if guessed or max trials reached
            }
            return 1;
        }
    }
    return 0; // No active game found for this player
}

// Handle the `quit` command
void quit_game(const char *plid, char *response) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (active_games[i].active && strcmp(active_games[i].plid, plid) == 0) {
            active_games[i].active = 0;
            snprintf(response, BUFFER_SIZE, "RQT OK %s\n", active_games[i].secret_key);
            return;
        }
    }
    snprintf(response, BUFFER_SIZE, "RQT NOK\n");   // No active game found
}

// Handle incoming UDP messages
void handle_udp_message(int udp_socket, struct sockaddr_in *client_addr, socklen_t client_len, char *buffer) {
    printf("Received message: %s", buffer);  // Log the received message
    char response[BUFFER_SIZE];
    memset(response, 0, BUFFER_SIZE);

    // Parse and handle the message based on the command type
    if (strncmp(buffer, "SNG", 3) == 0) {   // Start New Game
        char plid[7], secret_key[5];
        int max_playtime;

        if (sscanf(buffer, "SNG %6s %3d", plid, &max_playtime) == 2) {
            if (max_playtime > 600) {
                snprintf(response, sizeof(response), "RSG ERR\n");  // Invalid playtime
            } else if (start_new_game(plid, max_playtime, secret_key)) {
                snprintf(response, sizeof(response), "RSG OK\n");   // Game started successfully
            } else {
                snprintf(response, sizeof(response), "RSG NOK\n");  // Failed to start game
            }
        } else {
            snprintf(response, sizeof(response), "RSG ERR\n");      // Invalid syntax
        }

    } else if (strncmp(buffer, "TRY", 3) == 0) {    // Process Guess
        char plid[7], guess[10];
        int nB, nW;

        if (sscanf(buffer, "TRY %6s %s", plid, guess) == 2) {
            if (process_guess(plid, guess, &nB, &nW)) {
                snprintf(response, sizeof(response), "RTR OK %d %d\n", nB, nW); // Guess processed
            } else {
                snprintf(response, sizeof(response), "RTR NOK\n");  // Invalid guess or no active game
            }
        } else {
            snprintf(response, sizeof(response), "RTR ERR\n");  // Invalid syntax
        }

    } else if (strncmp(buffer, "QUT", 3) == 0) {    // Quit Game
        char plid[7];
        if (sscanf(buffer, "QUT %6s", plid) == 1) {
            quit_game(plid, response);  // Process quit request
        } else {
            snprintf(response, sizeof(response), "RQT ERR\n");   // Invalid syntax
        }

    } else {
        snprintf(response, sizeof(response), "ERR\n");  // Unknown command
    }

    // Send the response back to the client
    sendto(udp_socket, response, strlen(response), 0, (struct sockaddr*)client_addr, client_len);
    printf("Sent response: %s\n", response);
}

// Handle incoming TCP connections
void handle_tcp_connection(int client_socket) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    // Read the client's message
    read(client_socket, buffer, BUFFER_SIZE);
    printf("Received TCP message: %s\n", buffer);

    if (strncmp(buffer, "STR", 3) == 0) {   // Show Trials
        char trials[1024];
        get_trials("123456", trials); // Replace "123456" with actual logic to extract PLID
        write(client_socket, trials, strlen(trials));   // Send the trial summary

    } else if (strncmp(buffer, "SSB", 3) == 0) {    // Show Scoreboard
        char scores[1024];
        get_scoreboard(scores); // Generate scoreboard data
        write(client_socket, scores, strlen(scores));

    } else {
        write(client_socket, "ERR\n", 4);   // Unknown command
    }
}

// Generate a trial summary for a given player (PLID)
void get_trials(const char *plid, char *buffer) {
    snprintf(buffer, BUFFER_SIZE, "Trials for player %s:\n1. RGBY -> 2B 1W\n2. RBYO -> 1B 0W\n", plid);
}

// Generate the scoreboard
void get_scoreboard(char *buffer) {
    snprintf(buffer, BUFFER_SIZE, "Top-10 Players:\n1. 123456 - 5 trials - RGBY\n2. 654321 - 4 trials - BYRG\n");
}