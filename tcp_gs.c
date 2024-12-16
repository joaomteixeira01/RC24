/**
 * This file: 
 *  - Manages stateful requests such as retrieving game trials (STR) and the scoreboard (SSB).
 *  - Uses read and write to communicate with the client over a connected socket.
 */

#include "server.h"

/**
 * Starts the TCP server.
 * This Function creates a TCP socket, binds it to the specified port, listens for incoming connections, 
 * and processes client requests for trials (`STR`) and scoreboard (`SSB`).
 */
void start_tcp_server() {
    int tcp_socket, new_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Create a TCP socket
    tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_socket < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Configure server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;               // IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY;       // Accept connections from any address
    server_addr.sin_port = htons(atoi(TCP_PORT));   // Set server port

    // Bind the socket to the server address
    if (bind(tcp_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(tcp_socket);
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
    listen(tcp_socket, MAX_CLIENTS);
    printf("TCP Server running on port %s...\n", TCP_PORT);

    // Main loop to accept and handle client connections
    while (1) {
        // Accept a new client connection
        new_socket = accept(tcp_socket, (struct sockaddr*)&client_addr, &client_len);
        if (new_socket < 0) {
            perror("accept");
            continue;
        };

        printf("New client connected!\n");

        // Handle the client request
        handle_tcp_connection(new_socket);

        // Close the connection with the client
        close(new_socket);
    }

    // Close the socket when done (this part is never reached in this code)
    close(tcp_socket);
}

/**
 * Handles a single TCP connection.
 * Reads a message from the client, determines the command (`STR` or `SSB`), processes it,
 * and sends an appropriate response back to the client.
 */
void handle_tcp_connection(int tcp_socket) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);  // Clear the buffer

    // Read a message from the client
    ssize_t bytes_read = read(tcp_socket, buffer, BUFFER_SIZE);
    if (bytes_read <= 0) {
        perror("read");
        return;
    }

    printf("Received TCP message: %s\n", buffer);

    // Determine the command and process it
    if (strncmp(buffer, "STR", 3) == 0) {  // "STR" -> Show Trials
        char trials[1024];
        get_trials("123456", trials);  // Replace "123456" with actual logic to extract PLID
        write(tcp_socket, trials, strlen(trials));  // Send the trials summary
    } else if (strncmp(buffer, "SSB", 3) == 0) {  // "SSB" -> Show Scoreboard
        char scores[1024];
        get_scoreboard(scores);  // Generate the scoreboard
        write(tcp_socket, scores, strlen(scores));  // Send the scoreboard
    } else {
        // If the command is unknown, send an error message
        write(tcp_socket, "ERR\n", 4);
    }
}

/**
 * Generates the trial summary for a given player (PLID).
 * This function simulates creating a summary of all trials made by the player.
 * Replace with actual game logic as necessary.
 */
void get_trials(const char *plid, char *buffer) {
    snprintf(buffer, BUFFER_SIZE, "Trials for player %s:\n1. RGBY -> 2B 1W\n2. RBYO -> 1B 0W\n", plid);
    // Example output for demonstration purposes. Replace with real data from game state.
}

/**
 * Generates the scoreboard.
 * This function simulates creating a scoreboard with the top-10 players.
 * Replace with actual game logic as necessary.
 */
void get_scoreboard(char *buffer) {
    snprintf(buffer, BUFFER_SIZE, 
        "Top-10 Players:\n1. 123456 - 5 trials - RGBY\n2. 654321 - 4 trials - BYRG\n");
    // Example output for demonstration purposes. Replace with real data from game state.
}
