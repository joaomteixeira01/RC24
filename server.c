 /* 
 * server.c
 * 
 * This is a combined server for handling both UDP and TCP connections in a simple Mastermind-style game. 
 * It uses select to manage multiple clients at the same time without blocking.
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
#include <dirent.h>

// Array to store active games for all connected clients
Game active_games[MAX_CLIENTS];

char formatted_key[10];

int verbose = 0;

// ====================== Create files ======================

// Function to create the required directories ("GAMES" and "SCORES")
void create_directories() {
    // Attempt to create the "GAMES" directory
    if (mkdir("GAMES", 0777) == -1) {
        perror("Failed to create directory GAMES");
    } else {
        if (verbose) printf("Directory GAMES created successfully.\n");
    }

    // Attempt to create the "SCORES" directory
    if (mkdir("SCORES", 0777) == -1) {
        perror("Failed to create directory SCORES");
    } else {
         if (verbose) printf("Directory SCORES created successfully.\n");
    }
}

/* ---------------- GAMES ---------------- */ 
// Function to create the initial game state file
void create_game_file(const char *plid, char mode, const char *code, int max_time) {
    char game_dir[100], filename[150];

    // Criar o diretório GAMES/<PLID>
    sprintf(game_dir, "GAMES/%s", plid);
    if (mkdir(game_dir, 0777) == -1 && errno != EEXIST) {
        perror("Failed to create player directory");
        return;
    }

    // Criar o nome do arquivo GAME_<PLID>.txt
    sprintf(filename, "%s/GAME_%s.txt", game_dir, plid);

    // Abrir o arquivo para escrita
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Failed to create game file");
        return;
    }

    // Obter o timestamp atual
    time_t current_time = time(NULL);
    struct tm *tm_info = gmtime(&current_time);

    // Formatar o timestamp
    char time_str[20];
    strftime(time_str, 20, "%Y-%m-%d %H:%M:%S", tm_info);

    // Escrever o estado inicial do jogo
    fprintf(file, "%s %c %s %d %s %ld\n", plid, mode, code, max_time, time_str, current_time);
    fclose(file); // Fechar o arquivo
    if (verbose) printf("Game file created: %s\n", filename);
}


// Function to append a trial to the game file
void add_trial(const char *plid, const char *guess, int correct_pos, int wrong_pos, int elapsed_time) {
    char filename[50];
    sprintf(filename, "GAMES/%s/GAME_%s.txt", plid, plid); // Format the filename based on PLID

    // Open the file in append mode
    FILE *file = fopen(filename, "a");
    if (!file) {
        perror("Failed to open game file for appending");
        return;
    }

    // Write the trial data to the file (Format => T: CCCC B W s)
    fprintf(file, "T: %s %d %d %d\n", guess, correct_pos, wrong_pos, elapsed_time);
    fclose(file);
    if (verbose) printf("Trial added to game file: %s\n", filename);
}


// Function to finalize the game, move its file, and rename it
void finish_game(const char *plid, const char *end_code) {
    char game_dir[100], current_filename[150], final_filename[150];
    char final_date[9], final_time[7];
    int game_duration = 0, i = 0;
    // Obter a data e a hora atuais
    time_t current_time = time(NULL);
    struct tm *tm_info = gmtime(&current_time);

    // Formatar a data e a hora
    strftime(final_date, sizeof(final_date), "%Y%m%d", tm_info); // Formato: YYYYMMDD
    strftime(final_time, sizeof(final_time), "%H%M%S", tm_info); // Formato: HHMMSS

    // Diretório do jogador
    sprintf(game_dir, "GAMES/%s", plid);

    // Nome do arquivo atual (GAME_<PLID>.txt)
    sprintf(current_filename, "%s/GAME_%s.txt", game_dir, plid);

    // Nome do arquivo final (YYYYMMDD_HHMMSS_(code).txt)
    sprintf(final_filename, "%s/%s_%s_%s.txt", game_dir, final_date, final_time, end_code);

    // Abrir o arquivo atual para adicionar a última linha
    FILE *file = fopen(current_filename, "a");
    if (file) {
        // Formatar a linha final: "2024-12-20 17:11:06 52"
        char final_date_time[20];
        strftime(final_date_time, sizeof(final_date_time), "%Y-%m-%d %H:%M:%S", tm_info);

        // Calcular a duração do jogo

        for (i = 0; i < MAX_CLIENTS; i++) {
            if (strcmp(active_games[i].plid, plid) == 0) {
                game_duration = (int)difftime(current_time, active_games[i].start_time);
                active_games[i].active = 0; // Finalizar o jogo
                break;
            }
        }
        if (verbose) printf("game duration: %d\n", game_duration);

        // Escrever a última linha no arquivo
        fprintf(file, "%s %d\n", final_date_time, game_duration);
        fclose(file);
    } else {
        perror("Failed to append to game file");
    }

    // Renomear o arquivo para o formato final
    if (rename(current_filename, final_filename) == 0) {
        if (verbose) printf("Game file renamed to: %s\n", final_filename);
    } else {
        perror("Failed to rename game file");
    }
    if (strncmp(end_code, "W", 1) == 0) {
        create_score_file(plid, active_games[i].secret_key, active_games[i].trials, active_games[i].mode, game_duration, active_games[i].max_playtime);
    }
}




/* ---------------- SCORES ---------------- */ 
void create_score_file(const char *plid, const char *code, int trials, const char *mode, int duration, int max_playtime) {
    char filename[100];
    char date_str[20], time_str[20];
    time_t current_time = time(NULL);
    struct tm *tm_info = gmtime(&current_time); // UTC
    int score;

    score = (100 - (((float)(trials - 1) / 7) * 50)) * (1 - ((float)duration / max_playtime) * 0.5); // Calculate the score
    // Formatar data e hora
    strftime(date_str, sizeof(date_str), "%d%m%Y", tm_info); // Formato: DDMMYYYY
    strftime(time_str, sizeof(time_str), "%H%M%S", tm_info); // Formato: HHMMSS

    // Nome do arquivo
    sprintf(filename, "SCORES/%03d_%s_%s_%s.txt", score, plid, date_str, time_str);

    // Abrir o arquivo para escrita
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Failed to create score file");
        return;
    }

    // Escrever os dados no formato especificado
    fprintf(file, "%03d %s %s %d %s\n", score, plid, code, trials, mode);
    fclose(file);

    if (verbose) printf("Score file created: %s\n", filename);
}



// =====================================================

int main(int argc, char *argv[]) {
    int udp_socket, tcp_socket, max_fd, gsport;
    struct sockaddr_in udp_addr, tcp_addr, client_addr;
    fd_set read_fds;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len;
    
    gsport = PORT;
    
    int opt;
    while ((opt = getopt(argc, argv, "p:v"))!= -1) {
        switch (opt) {
            case 'p':
                gsport = atoi(optarg);
                break;
            case 'v':
                verbose = 1;
                break;
            default:
                printf("Usage: GS [-p port] [-v]\n");
                exit(1);
        }
    }

    // Create GAMES and SCORES directories
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
    udp_addr.sin_port = htons(gsport);    // Set the UDP port

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
    tcp_addr.sin_port = htons(gsport);    // Set the TCP port

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

    if (verbose) printf("Server running on port %d\n", gsport);

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

            if (verbose) printf("Waiting for UDP message...\n"); 

            ssize_t n = recvfrom(udp_socket, buffer, BUFFER_SIZE, 0, 
                                (struct sockaddr*)&client_addr, &addr_len);
            if (n < 0) {
                perror("recvfrom failed");
            } else {
                buffer[n] = '\0'; // Null-terminate received data
                if (verbose) printf("Received UDP message: %s", buffer);  // Confirm reception
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
            if (verbose) printf("New TCP client connected\n");
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
int start_new_game(const char *plid, int max_playtime, char *secret_key, char *mode) {
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
            strcpy(active_games[i].mode, mode);
            active_games[i].max_playtime = max_playtime;
            active_games[i].trials = 0;
            active_games[i].active = 1;
            active_games[i].start_time = time(NULL); // Record the start time
            if (strlen(secret_key) == 0){
                generate_secret_key(active_games[i].secret_key);
                strcpy(secret_key, active_games[i].secret_key);
            }
            else{
                strcpy(active_games[i].secret_key, secret_key);
                snprintf(formatted_key, 10, "%c %c %c %c", secret_key[0], secret_key[1], secret_key[2], secret_key[3]);
                formatted_key[7] = '\0';
            }
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

            *nB = *nW = 0;
            int color_counts[6] = {0};
            const char colors[] = "RGBYOP";

            // Check elapsed time
            time_t current_time = time(NULL);
            int elapsed_time = (int)difftime(current_time, active_games[i].start_time);

            if (verbose) {printf("Player: %s, Start Time: %ld, Current Time: %ld, Elapsed Time: %d seconds\n", 
                   plid, active_games[i].start_time, current_time, elapsed_time);}

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

            /**
             * TODO: 
             *  - Resend scenario
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
            add_trial(plid, guess, *nB, *nW, elapsed_time); // Pass the start time

            if (*nB == 4) {
                active_games[i].active = 0;
                return 1; // Game won
            }

            if (active_games[i].trials >= MAX_ATTEMPTS) {
                active_games[i].active = 0;
                format_secret_key(active_games[i].secret_key, secret_key);
                return 2; // Game over: Maximum attempts reached
            }

            return 0; // OK: Valid guess
        }
    }

    return -4; // NOK: No active game found
}

// Handle the quit command
void quit_game(const char *plid, char *response) {
    char secret_key[10];
    int game_found = 0;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (active_games[i].active && strcmp(active_games[i].plid, plid) == 0) {
            game_found = 1;
            active_games[i].active = 0;
            finish_game(plid, "Q"); // Finalize the game using finish_game with "Q" for quit
            format_secret_key(active_games[i].secret_key, secret_key);
            snprintf(response, BUFFER_SIZE, "RQT OK %s\n", secret_key);
            return;
        }
    }
    if (!game_found) {
        snprintf(response, BUFFER_SIZE, "RQT NOK\n"); // No active game found
    }
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
        char plid[7];

        if (sscanf(buffer, "SNG %6s %3d", plid, &max_playtime) == 2) {
            if (max_playtime > MAX_PLAYTIME) {
                snprintf(response, sizeof(response), "RSG ERR\n");  // Invalid playtime
            } else if (start_new_game(plid, max_playtime, secret_key, "PLAY")) {
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

        char key[5] = {c1, c2, c3, c4, '\0'};
        //int res = processDebug(plid, &max_playtime, guess, secret_key) implement processDebug on another aux file to clean the code
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
                if (start_new_game(plid, max_playtime, key, "DEBUG")) {
                    create_game_file(plid, 'D', key, max_playtime);  // Create the game file
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
    if (verbose) printf("Sent response: %s\n", response);
}

// Handle incoming TCP connections
void handle_tcp_connection(int client_socket) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    // Read the client's message
    read(client_socket, buffer, BUFFER_SIZE);
    if (verbose) printf("Received TCP message: %s\n", buffer);

    // Variable to store extracted PLID
    char plid[7]; // 6 characters + null terminator
    memset(plid, 0, sizeof(plid));

    // ------------------ Show trials ------------------
    if (strncmp(buffer, "STR", 3) == 0) {   
        // char trials[1024];
        // get_trials("123456", trials); // Replace "123456" with the logic to extract PLID
        // write(client_socket, trials, strlen(trials));   // Send the trial summary
    }
    // FIXME!!! Not sure about this resolution
    if (strncmp(buffer, "STR", 3) == 0) {
        // Extract PLID from the message
        if (sscanf(buffer, "STR %6s", plid) == 1) {
            char trials[1024];
            get_trials(plid, trials); // Use the extracted PLID
            write(client_socket, trials, strlen(trials)); // Send the trial summary
        } else {
            write(client_socket, "RST NOK\n", 8); // Invalid syntax
        }
    // ------------------ Show Scoreboard ------------------
    } else if (strncmp(buffer, "SSB", 3) == 0) {   
        char scores[1024];
        get_scoreboard(scores); // Generate scoreboard data
        write(client_socket, scores, strlen(scores));

    } else {
        write(client_socket, "ERR\n", 4);   // Unknown command
    }

    close(client_socket);// Close the socket for both reading and writing
}

// Generate a trial summary for a given player (PLID)
void get_trials(const char *plid, char *buffer) {
    int game = get_game(plid);
    char fname[100], formatted_fname[25];
    char line[100];
    strcpy(buffer, "\0");
    if (game == -1){ 
        if (verbose) printf("No active game found for player %s, using last game\n", plid);
        if(!find_last_game(plid, fname)) {
            if (verbose) printf("No game found for player %s\n", plid);
            sprintf(buffer, "RST NOK\n");
            return;
        }
        sscanf(fname, "GAMES/%*s/%s", formatted_fname);
        FILE *file = fopen(fname, "r");
        if (!file) {
            perror("Failed to open game file for reading");
            return;
        }
        fseek(file, 0L, SEEK_END);
        long int size = ftell(file);
        rewind(file);
        sprintf(buffer, "RST FIN %s %ld ", fname, size);
        while (fgets(line, sizeof(line), file))
            strcat(buffer, line);

        fclose(file);
    } else {
        if (verbose) printf("Active game found for player %s\n", plid);
        sprintf(formatted_fname, "GAME_%s.txt", plid);
        sprintf(fname, "GAMES/%s/GAME_%s.txt", plid, plid);
        FILE *file = fopen(fname, "r");
        if (!file) {
            perror("Failed to open game file for reading");
            return;
        }
        fseek(file, 0L, SEEK_END);
        long int size = ftell(file);
        rewind(file);
        sprintf(buffer, "RST ACT %s %ld ", formatted_fname, size);
        while (fgets(line, sizeof(line), file)) 
            strcat(buffer, line);

        fclose(file);
    }
}

// Generate the scoreboard
void get_scoreboard(char *buffer) {
    Scorelist list;
    if(!find_top_scores(&list)) {
        if (verbose) printf("No scores found\n");
        sprintf(buffer, "RSS EMPTY\n"); 
        return;
    }
    if (verbose) printf("Scores found\n");
    char line[100];
    FILE *scoreboard_file = fopen("SCORES/scoreboard.txt", "w");
    if (!scoreboard_file) {
        perror("Failed to create scoreboard file");
        sprintf(buffer, "RSS ERR\n");
        return;
    }

    for (int i = 0; i < list.n_scores; i++) {
        fprintf(scoreboard_file, "%03d %s %s %d %s\n", list.score[i], list.plid[i], list.secret_key[i], list.no_trials[i], list.mode[i]);
    }
    fclose(scoreboard_file);

    // Read the scoreboard file to send its content
    scoreboard_file = fopen("SCORES/scoreboard.txt", "r");
    if (!scoreboard_file) {
        perror("Failed to open scoreboard file for reading");
        sprintf(buffer, "RSS ERR\n");
        return;
    }

    fseek(scoreboard_file, 0L, SEEK_END);
    long int size = ftell(scoreboard_file);
    rewind(scoreboard_file);

    sprintf(buffer, "RSS OK scoreboard.txt %ld ", size);
    while (fgets(line, sizeof(line), scoreboard_file)) {
        strcat(buffer, line);
    }
    fclose(scoreboard_file);

}

int get_game(const char *plid) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (active_games[i].active && strcmp(active_games[i].plid, plid) == 0) {
            return i;
        }
    }
    return -1;
}

int find_last_game(const char *plid, char* fname) {
    struct dirent **filelist;
    int n_entries, found;
    char dirname[20];

    sprintf(dirname, "GAMES/%s", plid);
    n_entries = scandir(dirname, &filelist, 0, alphasort);

    found = 0;

    if (n_entries <= 0)
        return 0;

    while (n_entries--) {
        if(filelist[n_entries]->d_name[0] != '.' && !found) {
            sprintf(fname, "GAMES/%s/%s", plid, filelist[n_entries]->d_name);
            found = 1;
        }
        free(filelist[n_entries]);
    }
    free(filelist);
    return found;
}

int find_top_scores(Scorelist *list){
    struct dirent **filelist;
    int n_entries, i_file;
    char fname[300];
    FILE *file;

    n_entries = scandir("SCORES", &filelist, 0, alphasort);

    if (n_entries <= 0)
        return 0;

    i_file = 0;
    while(n_entries--){
        if (filelist[n_entries]->d_name[0] != '.' && i_file < 10){
            sprintf(fname, "SCORES/%s", filelist[n_entries]->d_name);
            file = fopen(fname, "r");
            if (!file) {
                perror("Failed to open score file for reading");
                return 0;
            }
            fscanf(file, "%d %s %s %d %s", &list->score[i_file], list->plid[i_file], list->secret_key[i_file], &list->no_trials[i_file], list->mode[i_file]);
            fclose(file);
            i_file++;
        } 
        free(filelist[n_entries]);
    }
    
    free(filelist);
    list->n_scores = i_file;
    return(i_file);
}