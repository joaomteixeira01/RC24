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
            active_games[i].start_time = time(NULL); // Record the start time
            generate_secret_key(active_games[i].secret_key);
            strcpy(secret_key, active_games[i].secret_key);
            return 1;
        }
    }
    return 0; // No available slots
}

// Utility function to format the secret_key with spaces between each character
void format_secret_key(const char *secret_key, char *formatted_key) {
    snprintf(formatted_key, 10, "%c %c %c %c", secret_key[0], secret_key[1], secret_key[2], secret_key[3]);
    formatted_key[7] = '\0'; // Null-terminate to avoid garbage data
}


// Process a player's guess and update the game state
int process_guess(const char *plid, const char *guess, int nT, int *nB, int *nW, char *secret_key) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (active_games[i].active && strcmp(active_games[i].plid, plid) == 0) {

            int color_counts[6] = {0};
            const char colors[] = "RGBYOP";

            // Check elapsed time
            time_t current_time = time(NULL);
            if (difftime(current_time, active_games[i].start_time) > active_games[i].max_playtime) {
                active_games[i].active = 0; // End the game
                format_secret_key(active_games[i].secret_key, secret_key);
                return -5; // Time exceeded
            }

            // Validate PLID
            if (strlen(plid) != 6 || strspn(plid, "0123456789") != 6) {
                return -1; // ERR: Invalid PLID
            }

            // Validate the color codes in the guess
            for (int i = 0; i < 4; i++) {
                if (!strchr(colors, guess[i])) {
                    return -1; // ERR: Invalid color code
                }
            }

            *nB = *nW = 0;

            /**
             *  TODO: 
             *      - Resend scenario
            */

            if (nT != active_games[i].trials + 1) {
                if (nT == active_games[i].trials &&
                    strcmp(active_games[i].guesses[active_games[i].trials - 1], guess) == 0) {
                    return 0; // OK: Resending the last valid guess
                }
                return -2; // INV: Invalid trial number
            }

            for (int j = 0; j < active_games[i].trials; j++) {
                if (strcmp(active_games[i].guesses[j], guess) == 0) {
                    return -3; // DUP: Duplicate guess
                }
            }

            for (int j = 0; j < 4; j++) {
                if (guess[j] == active_games[i].secret_key[j]) {
                    (*nB)++;
                } else {
                    for (int k = 0; k < 6; k++) {
                        if (active_games[i].secret_key[j] == colors[k]) {
                            color_counts[k]++;
                            break;
                        }
                    }
                }
            }

            for (int j = 0; j < 4; j++) {
                if (guess[j] != active_games[i].secret_key[j]) {
                    for (int k = 0; k < 6; k++) {
                        if (guess[j] == colors[k] && color_counts[k] > 0) {
                            (*nW)++;
                            color_counts[k]--;
                            break;
                        }
                    }
                }
            }

            strncpy(active_games[i].guesses[active_games[i].trials], guess, 4);
            active_games[i].guesses[active_games[i].trials][4] = '\0';
            active_games[i].trials++;

            if (*nB == 4) {
                active_games[i].active = 0;
                return 1; // Game won
            }

            if (active_games[i].trials > MAX_ATTEMPTS) {
                active_games[i].active = 0;
                format_secret_key(active_games[i].secret_key, secret_key);
                return 2; // Game over: Maximum attempts reached
            }

            return 0; // OK: Valid guess
        }
    }

    return -4; // NOK: No active game found
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
    char response[BUFFER_SIZE];
    memset(response, 0, BUFFER_SIZE);
    char secret_key[5] = {0};

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
        char plid[7], c1, c2, c3, c4;
        int nB, nW, nT;

        if (sscanf(buffer, "TRY %6s %c %c %c %c %d", plid, &c1, &c2, &c3, &c4, &nT) == 6) {
            // Ensure the format is valid (check spaces and colors)
            if (buffer[10] != ' ' || buffer[12] != ' ' || buffer[14] != ' ' || buffer[16] != ' ' || buffer[18] != ' ') {
                snprintf(response, sizeof(response), "RTR INV\n"); // Invalid format
            } else if (!strchr("RGBYOP", c1) || !strchr("RGBYOP", c2) || !strchr("RGBYOP", c3) || !strchr("RGBYOP", c4)) {
                snprintf(response, sizeof(response), "RTR INV\n");
            } else {
                // Compact the guess into a single string
                char guess[5] = {c1, c2, c3, c4, '\0'};

                int result = process_guess(plid, guess, nT, &nB, &nW, secret_key);

                switch (result) {
                    case 0: // OK: Valid guess
                        snprintf(response, sizeof(response), "RTR OK %d %d %d\n", nT, nB, nW);
                        break;

                    case 1: // Game won
                        snprintf(response, sizeof(response), "RTR OK %d %d %d\n", nT, nB, nW);
                        break;

                    case 2: // Game over: Maximum attempts reached
                        snprintf(response, sizeof(response), "RTR ENT %s\n", secret_key);
                        break;

                    case -1: // ERR: Invalid guess
                        snprintf(response, sizeof(response), "RTR ERR\n");
                        break;

                    case -2: // INV: Invalid trial number
                        snprintf(response, sizeof(response), "RTR INV\n");
                        break;

                    case -3: // DUP: Duplicate guess
                        snprintf(response, sizeof(response), "RTR DUP\n");
                        break;

                    case -4: // NOK: No active game found
                        snprintf(response, sizeof(response), "RTR NOK\n");
                        break;
                    
                    case -5: // ETM: Time exceeded
                        snprintf(response, sizeof(response), "RTR ETM %s\n", secret_key); 
                        break; 

                    case -6: // ENT: Max attempts reached

                    default:
                        snprintf(response, sizeof(response), "RTR ERR\n");
                        break;
                }
            }
        } else {
            // Invalid syntax
            snprintf(response, sizeof(response), "RTR ERR\n");
        }

    } else if (strncmp(buffer, "QUT", 3) == 0) {    // Quit Game
        char plid[7];

        /**
         * TODO:
         *  - If PLID did not have an ongoing game, the GS replies with status = NOK
         */

        if (sscanf(buffer, "QUT %6s", plid) == 1) {
            snprintf(response, sizeof(response), "RQT OK %s\n", secret_key); 
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
        get_trials("123456", trials); // Replace "123456" with the logic to extract PLID
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