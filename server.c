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
#include <sys/stat.h>
#include <sys/types.h>

// Array to store active games for all connected clients
Game active_games[MAX_CLIENTS];

char formatted_key[10];

// ====================== Create files ======================

// Function to create the required directories ("GAMES" and "SCORES")
void create_directories() {
    // Attempt to create the "GAMES" directory
    if (mkdir("GAMES", 0777) == -1) {
        perror("Failed to create directory GAMES");
    } else {
        printf("Directory GAMES created successfully.\n");
    }

    // Attempt to create the "SCORES" directory
    if (mkdir("SCORES", 0777) == -1) {
        perror("Failed to create directory SCORES");
    } else {
        printf("Directory SCORES created successfully.\n");
    }
}

/* ---------------- GAMES ---------------- */ 
// Function to create the initial game state file
void create_game_file(const char *plid, char mode, const char *code, int max_time) {
    char filename[50];
    sprintf(filename, "GAMES/%s.txt", plid); // Format the filename based on PLID

    // Open the file for writing
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Failed to create game file");
        return;
    }

    // Get the current timestamp
    time_t current_time = time(NULL);
    struct tm *tm_info = gmtime(&current_time); // Convert to UTC time

    // Format the timestamp into a human-readable string
    char time_str[20];
    strftime(time_str, 20, "%Y-%m-%d %H:%M:%S", tm_info);

    // Write the initial game state to the file
    fprintf(file, "%s %c %s %d %s %ld\n", plid, mode, code, max_time, time_str, current_time);
    fclose(file); // Close the file
    printf("Game file created: %s\n", filename);
}

// Function to append a trial to the game file
void add_trial(const char *plid, const char *guess, int correct_pos, int wrong_pos, int elapsed_time) {
    char filename[50];
    sprintf(filename, "GAMES/%s.txt", plid); // Format the filename based on PLID

    // Open the file in append mode
    FILE *file = fopen(filename, "a");
    if (!file) {
        perror("Failed to open game file for appending");
        return;
    }

    // Write the trial data to the file (Format => T: CCCC B W s)
    fprintf(file, "T: %s %d %d %d\n", guess, correct_pos, wrong_pos, elapsed_time);
    fclose(file);
    printf("Trial added to game file: %s\n", filename);
}

// Function to finalize the game, move its file, and rename it
void finish_game(const char *plid, const char *end_code) {
    char filename[50];
    sprintf(filename, "GAMES/%s.txt", plid); // File name: GAMES/(plid).txt

    // Get the game index using `get_game`
    int game_id = get_game(plid);
    if (game_id == -1) {
        printf("No active game found for PLID: %s\n", plid);
        return;
    }

    // Access the game data
    Game *game = &active_games[game_id];

    // Get the current timestamp
    time_t current_time = time(NULL);
    struct tm *tm_info = gmtime(&current_time); // UTC

    // Format date and time
    char date_time_str[20];
    strftime(date_time_str, 20, "%Y-%m-%d %H:%M:%S", tm_info); // Format: YYYY-MM-DD HH:MM:SS

    // Calculate game duration
    int game_duration = (int)difftime(current_time, game->start_time);
    int trials = game->trials;
    char mode = game->mode;

    // Open the file to append the final line
    FILE *file = fopen(filename, "a");
    if (!file) {
        perror("Failed to open game file for appending");
        return;
    }

    // Add the final line with date, time, and game duration
    fprintf(file, "%s %d\n", date_time_str, game_duration);
    fclose(file);

    // If the game ended successfully, rename the file and save the score
    if (strcmp(end_code, "W") == 0) {
        char new_filename[100];
        char final_date[9], final_time[7];

        // Format the date and time for the new file name
        strftime(final_date, 9, "%Y%m%d", tm_info); // Format: YYYYMMDD
        strftime(final_time, 7, "%H%M%S", tm_info); // Format: HHMMSS

        // Create the new file name
        sprintf(new_filename, "GAMES/%s_%s_W.txt", final_date, final_time);

        // Rename the file
        if (rename(filename, new_filename) == 0) {
            printf("Game file renamed to: %s\n", new_filename);
        } else {
            perror("Failed to rename game file");
        }

        // Calculate the score (example: 100 - 10 * number of trials)
        int score = 100 - (trials * 10);
        if (score < 0) score = 0;

        // Create the score file
        create_score_file(plid, end_code, trials, mode, score);
    }
}

/* ---------------- SCORES ---------------- */ 
void create_score_file(const char *plid, const char *code, int trials, const char *mode, int score) {
    char filename[100];
    char date_str[20], time_str[20];
    time_t current_time = time(NULL);
    struct tm *tm_info = gmtime(&current_time); // UTC

    // Formatar data e hora
    strftime(date_str, sizeof(date_str), "%d%m%Y", tm_info); // Formato: DDMMYYYY
    strftime(time_str, sizeof(time_str), "%H%M%S", tm_info); // Formato: HHMMSS

    // Nome do arquivo
    sprintf(filename, "SCORES/score_%s_%s_%s.txt", plid, date_str, time_str);

    // Abrir o arquivo para escrita
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Failed to create score file");
        return;
    }

    // Escrever os dados no formato especificado
    fprintf(file, "%03d %s %s %d %s\n", score, plid, code, trials, mode);
    fclose(file);

    printf("Score file created: %s\n", filename);
}



// =====================================================

int main() {
    int udp_socket, tcp_socket, max_fd;
    struct sockaddr_in udp_addr, tcp_addr, client_addr;
    fd_set read_fds;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len;

    // Criar os diretórios GAMES e SCORES
    create_directories();

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

            printf("Waiting for UDP message...\n"); 

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
    // for easy acess in some functions
    snprintf(formatted_key, 10, "%c %c %c %c", secret_key[0], secret_key[1], secret_key[2], secret_key[3]);
    formatted_key[7] = '\0'; 
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

    // Get the index of the active game using the get_game function
    int game_id = get_game(plid);
    if (game_id == -1) {
        return -4; // No active game found
    }

    // Access the active game using the retrieved index
    Game *game = &active_games[game_id];

    *nB = *nW = 0;                  // Initialize "nB" and "nW" to 0
    int color_counts[6] = {0};      // Array to track counts of each color in the secret key
    const char colors[] = "RGBYOP"; // Valid colors for the game

    // Check elapsed time for the game
    time_t current_time = time(NULL);
    int elapsed_time = (int)difftime(current_time, game->start_time);

    // Time exceeded
    if (elapsed_time > game->max_playtime) {
        game->active = 0; // Mark the game as inactive
        format_secret_key(game->secret_key, secret_key); // Format the secret key
        return -5; 
    }

    // Validate PLID
    if (strlen(plid) != 6 || strspn(plid, "0123456789") != 6) {
        return -1; // Invalid PLID
    }

    // Validate the color codes in the guess
    for (int i = 0; i < 4; i++) {
        if (!strchr(colors, guess[i])) {
            return -1; 
        }
    }

    // Validate the trial number
    if (nT != game->trials + 1) {
        if (nT == game->trials && strcmp(game->guesses[game->trials - 1], guess) == 0) {
            return 0; // Resending the last valid guess
        }
        return -2; // Invalid trial number
    }

    // Check for duplicate guesses
    for (int i = 0; i < game->trials; i++) {
        if (strcmp(game->guesses[i], guess) == 0) {
            return -3; // Duplicate guess
        }
    }

    // Calculate correct positions (nB) and correct colors in wrong positions (nW)
    for (int i = 0; i < 4; i++) {
        if (guess[i] == game->secret_key[i]) {
            (*nB)++;
        } else {
            for (int j = 0; j < 6; j++) {
                if (game->secret_key[i] == colors[j]) {
                    color_counts[j]++;
                    break;
                }
            }
        }
    }

    for (int i = 0; i < 4; i++) {
        if (guess[i] != game->secret_key[i]) {
            for (int j = 0; j < 6; j++) {
                if (guess[i] == colors[j] && color_counts[j] > 0) {
                    (*nW)++;
                    color_counts[j]--;
                    break;
                }
            }
        }
    }

    // Save the guess in the game's history
    strncpy(game->guesses[game->trials], guess, 4);
    game->guesses[game->trials][4] = '\0'; // Null-terminate the string
    game->trials++; // Increment trial count

    // Record the trial in the game file
    add_trial(plid, guess, *nB, *nW, elapsed_time);

    // Check if the guess matches the secret key
    if (*nB == 4) {
        game->active = 0; // Mark the game as inactive
        return 1; // Game won
    }

    // Check if maximum attempts have been reached
    if (game->trials > MAX_ATTEMPTS) {
        game->active = 0; // Mark the game as inactive
        format_secret_key(game->secret_key, secret_key); // Format the secret key
        return 2; // Maximum attempts reached
    }

    return 0; // Valid guess
}

//int process_debug(const char *plid, int *max_playtime, const char *guess) {}


// Handle the `quit` command
void quit_game(const char *plid, char *response) {
    int game_id = get_game(plid);

    if (game_id == -1) {
        snprintf(response, BUFFER_SIZE, "RQT NOK\n"); // No active game found
        return;
    }

    active_games[game_id].active = 0; // Mark teh game as inactive
    finish_game(plid, "Q"); // Finalize the game using `finish_game` with "Q" for quit
    snprintf(response, BUFFER_SIZE, "RQT OK %s\n", formatted_key);
}


// Handle incoming UDP messages
void handle_udp_message(int udp_socket, struct sockaddr_in *client_addr, socklen_t client_len, char *buffer) {
    char response[BUFFER_SIZE];
    memset(response, 0, BUFFER_SIZE);
    char secret_key[5] = {0};
    char plid[7];
    int max_playtime;
    char c1, c2, c3, c4;

    /*============= Parse and handle the message based on the command type =============*/ 
    
    // ------------------ Start New Game ------------------
    if (strncmp(buffer, "SNG", 3) == 0) {   
        char plid[7], secret_key[5];

        if (sscanf(buffer, "SNG %6s %3d", plid, &max_playtime) == 2) {
            if (max_playtime > MAX_PLAYTIME) {
                snprintf(response, sizeof(response), "RSG ERR\n");  // Invalid playtime
            } else if (start_new_game(plid, max_playtime, secret_key)) {
                create_game_file(plid, 'P', secret_key, max_playtime);  // Create the game file
                snprintf(response, sizeof(response), "RSG OK\n");   // Game started successfully
            } else {
                snprintf(response, sizeof(response), "RSG NOK\n");  // Failed to start game
            }
        } else {
            snprintf(response, sizeof(response), "RSG ERR\n");      // Invalid syntax
        }

    // ------------------ Try command ------------------
    } else if (strncmp(buffer, "TRY", 3) == 0) {    

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
                        finish_game(plid, "W");  // Finalize the game with "W" (Win)
                        snprintf(response, sizeof(response), "RTR OK %d %d %d\n", nT, nB, nW);
                        break;

                    case 2: // Game over: Maximum attempts reached
                        finish_game(plid, "F");  // Finalize the game with "F" (Fail)
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
                        finish_game(plid, "T");  // Finalize the game with "T" (Timeout)
                        snprintf(response, sizeof(response), "RTR ETM %s\n", secret_key); 
                        break; 

                    case -6: // ENT: Max attempts reached
                        // TODO!!

                    default:
                        snprintf(response, sizeof(response), "RTR ERR\n");
                        break;
                }
            }
        } else {
            // Invalid syntax
            snprintf(response, sizeof(response), "RTR ERR\n");
        }

    // ------------------ Quit Game ------------------
    } else if (strncmp(buffer, "QUT", 3) == 0) {   
        char plid[7];

        /**
         * TODO:
         *  - If PLID did not have an ongoing game, the GS replies with status = NOK
         */

        if (sscanf(buffer, "QUT %6s", plid) == 1) {
            snprintf(response, sizeof(response), "RQT OK %s\n", formatted_key); 
            quit_game(plid, response);  // Process quit request

        } else {
            snprintf(response, sizeof(response), "RQT ERR\n");   // Invalid syntax
        }

    // ------------------ Debug command ------------------
    } else if (sscanf(buffer, "DBG %6s %d %c %c %c %c", plid, &max_playtime, &c1, &c2, &c3, &c4) == 6) {

        //char guess[5] = {c1, c2, c3, c4, '\0'};
        //int res = processDebug(plid, &max_playtime, guess) implement processDebug on another aux file to clean the code
        //switch (res) {
            // Invalid PLID/playtime/color codes
        //    case -1:
        //        snprintf(response, sizeof(response), "RDB ERR\n");
        //        break;
            
            // Player already has an ongoing game OR Failed to start the game
        //    case 0:
        //        snprintf(response, sizeof(response), "RDB NOK\n");
        //        break;

            // Game successfully started
        //    case 1:
        //        snprintf(response, sizeof(response), "RDB OK\n");
        //        break;

        //    default:
        //        snprintf(response, sizeof(response), "RTR ERR\n");
        //        break;
        //}

        // Validate the PLID
        if (strlen(plid) != 6 || strspn(plid, "0123456789") != 6) {
            snprintf(response, sizeof(response), "RDB ERR\n");  // Invalid PLID
        }

        // Validate max_playtime
        else if (max_playtime <= 0 || max_playtime > MAX_PLAYTIME) {
            snprintf(response, sizeof(response), "RDB ERR\n");  // Invalid playtime
        }

        // Validate the colors
        else if (!strchr("RGBYOP", c1) || !strchr("RGBYOP", c2) || !strchr("RGBYOP", c3) || !strchr("RGBYOP", c4)) {
            snprintf(response, sizeof(response), "RDB ERR\n");  // Invalid color codes
        }
        
        // Check for an existing game
        else {
            int game_exists = 0;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (active_games[i].active && strcmp(active_games[i].plid, plid) == 0) {
                    game_exists = 1;
                    break;
                }
            }

            if (game_exists) {
                snprintf(response, sizeof(response), "RDB NOK\n");  // Player already has an ongoing game

            } else {
                // Create a new game with the specified secret key
                if (start_new_game(plid, max_playtime, secret_key)) {
                    snprintf(response, sizeof(response), "RDB OK\n");  // Game successfully started
                } else {
                    snprintf(response, sizeof(response), "RDB NOK\n");  // Failed to start the game
                }
            }
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

    // Variable to store extracted PLID
    char plid[7]; // 6 characters + null terminator
    memset(plid, 0, sizeof(plid));

    // ------------------ Show trials ------------------
    if (strncmp(buffer, "STR", 3) == 0) {   
        char trials[1024];
        get_trials("123456", trials); // Replace "123456" with the logic to extract PLID
        write(client_socket, trials, strlen(trials));   // Send the trial summary
    }
    // FIXME!!! Not sure about this resolution
    if (strncmp(buffer, "STR", 3) == 0) {
        // Extract PLID from the message
        if (sscanf(buffer, "STR %6s", plid) == 1) {
            char trials[1024];
            get_trials(plid, trials); // Use the extracted PLID
            write(client_socket, trials, strlen(trials)); // Send the trial summary
        } else {
            write(client_socket, "ERR\n", 4); // Invalid syntax
        }
    // ------------------ Show Scoreboard ------------------
    } else if (strncmp(buffer, "SSB", 3) == 0) {   
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


// -------------- AUX -----------------

// Função para encontrar o índice do jogo ativo pelo PLID
int get_game(const char *plid) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (active_games[i].active && strcmp(active_games[i].plid, plid) == 0) {
            return i; // Retorna o índice do jogo ativo
        }
    }
    return -1; // game not found
}