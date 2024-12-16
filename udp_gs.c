/**
 * Handles stateless requests like starting a game (SNG) and processing guesses (TRY).
 * Uses recvfrom and sendto for receiving and replying to client messages.
 */

#include "server.h"

// Array to store active games
Game active_games[MAX_CLIENTS];

/**
 * Starts the UDP server
 * This function creates a UDO socket, binds it to the specified port, and continuosly listens for incoming messages
 */
void start_udp_server() {
    int udp_socket;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    socklen_t client_len = sizeof(client_addr);

    // Create a UDP socket
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Configure server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;               // IPv4
    server_addr.sin_addr.s_addr =   INADDR_ANY;     // Accept connections from any address
    server_addr.sin_port = htons(atoi(UDP_PORT));   // Set server port

    // Bind the socket to the server address
    if (bind(udp_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(udp_socket);
        exit(EXIT_FAILURE);
    }

    print("UDP Server running on port %s...\n", UDP_PORT);

    // Main loop tp process incoming messages
    while(1) {
        memset(buffer, 0, BUFFER_SIZE); // Clear the buffer

        // Receibe a message from a client
        recvfrom(udp_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &client_len);
        printf("Received UDP message: %s\n", buffer);

        // Handle the received message
        handle_udp_message(udp_socket, &client_addr, client_len, buffer);
    }

    // Close the socket when done (this point will never be reached in this specific code)
    close(udp_socket);
}

/**
 * Handles incoming UDP messages.
 * Processes commands such as `SNG`, `TRY`, `QUT`, and `DBG` and sends appropriate responses to the client.
 */
void handle_udp_message(int udp_socket, struct sockaddr_in *client_addr, socklen_t client_len, char *buffer) {
    char response[BUFFER_SIZE];
    memset(response, 0, BUFFER_SIZE); // Clear the response buffer

    // Handle `SNG` command (Start New Game)
    if (strncmp(buffer, "SNG", 3) == 0) {
        char plid[7], secret_key[5];
        int max_playtime;
        if (sscanf(buffer, "SNG %6s %3d", plid, &max_playtime) == 2) {
            if (max_playtime > 600) {
                snprintf(response, sizeof(response), "RSG ERR\n");          // Invalid time
            } else if (start_new_game(plid, max_playtime, secret_key)) {
                snprintf(response, sizeof(response), "RSG OK\n");           // Successfully started
            } else {
                snprintf(response, sizeof(response), "RSG NOK\n");          // Failed to start
            }
        } else {
            snprintf(response, sizeof(response), "RSG ERR\n");              // Invalid syntax
        }
    }

    // Handle `TRY` command (Player's Guess)
    else if (strncmp(buffer, "TRY", 3) == 0) {
        char plid[7], guess[10];
        int nB, nW;
        if (sscanf(buffer, "TRY %6s %s", plid, guess) == 2) {
            if (process_guess(plid, guess, &nB, &nW)) {
                snprintf(response, sizeof(response), "RTR OK %d %d\n", nB, nW); // Valid guess
            } else {
                snprintf(response, sizeof(response), "RTR NOK\n");              // Invalid guess or no active game
            }
        } else {
            snprintf(response, sizeof(response), "RTR ERR\n");                  // Invalid syntax
        }
    }

    // Handle `QUT` command (Quit Game)
    else if (strncmp(buffer, "QUT", 3) == 0) {
        char plid[7];
        if (sscanf(buffer, "QUT %6s", plid) == 1) {
            quit_game(plid, response);  // Process quit command
        } else {
            snprintf(response, sizeof(response), "RQT ERR\n");  // Invalid syntax
        }
    }

    // Handle `DBG` command (Debug Mode)
    else if (strncmp(buffer, "DBG", 3) == 0) {
        char plid[7], secret_key[5];
        int max_playtime;
        if (sscanf(buffer, "DBG %6s %3d %4s", plid, &max_playtime, secret_key) == 3) {
            if (max_playtime > 600 || strlen(secret_key) != 4) {
                snprintf(response, sizeof(response), "RDB ERR\n");          // Invalid time or key
            } else if (start_new_game(plid, max_playtime, secret_key)) {
                snprintf(response, sizeof(response), "RDB OK\n");           // Debug game started
            } else {
                snprintf(response, sizeof(response), "RDB NOK\n");          // Failed to start
            }
        } else {
            snprintf(response, sizeof(response), "RDB ERR\n");              // Invalid syntax
        }
    }

    // Handle unknown commands
    else {
        snprintf(response, sizeof(response), "ERR\n");  // Unknown command
    }

    // Send the response back to the client
    sendto(udp_socket, response, strlen(response), 0, (struct sockaddr*)client_addr, client_len);
}