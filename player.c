/*
 * This file implements the main function for a client application that communicates 
 * with a game server via UDP and TCP. The application processes user commands and 
 * interacts with the server to manage game sessions. 
 *
 * Main Features:
 * - Processes command-line arguments to configure the server's IP and port.
 * - Initializes TCP and UDP sockets for communication with the server.
 * - Implements a command-line interface to handle the following user commands:
 *   - start: Starts a new game session with the server.
 *   - try: Submits a guess for the game.
 *   - show_trials (or st): Retrieves and displays trial information via TCP.
 *   - scoreboard (or sb): Retrieves the scoreboard via TCP.
 *   - quit: Ends the current game session.
 *   - exit: Exits the client application, optionally notifying the server.
 *   - debug: Sends debugging data to the server for testing.
 */

#include "client.h"
#include "command_handlers.h"

int main (int argc, char** argv){
    char *gs_ip = NULL;
    int gs_port = 58053;
    int fdudp, fdtcp;
    struct addrinfo *resudp, *restcp;
    char command[256];

    // Process command-line arguments
    int opt;
    while ((opt = getopt(argc, argv, "n:p:")) != -1) {
        switch (opt) {
            case 'n':
                gs_ip = optarg;
                break;
            
            case 'p':
                gs_port = atoi(optarg);
                break;
            
            default:
                fprintf(stderr, "Usage: %s [-n GSIP] [-n GSport]\n", argv[0]);
                exit(1);
        }
    }

    // Initialize sockets
    if (initialize_sockets(&fdtcp, &fdudp, &restcp, &resudp, gs_ip, gs_port) == -1) {
        fprintf(stderr, "Failed to initialize sockets\n");
        exit(1);
    }

    // Main command loop to process user input
    while (1) {
        printf("> ");
        if (fgets(command, sizeof(command), stdin) == NULL)
            break; // Exit the loop if input is invalid 

        //----------- Parse commands -----------//
        /* start command */
        if (strncmp(command, "start", 5) == 0) {
            char plid[7];   // Buffer for the PLID (6 digits + null terminator)
            int max_playtime;
            if (sscanf(command, "start %6s %d", plid, &max_playtime) == 2)
                handle_start(fdudp, resudp, plid, max_playtime);
            else
                printf("Usage: start PLID max_playtime\n");

        /* try command */
        } else if (strncmp(command, "try", 3) == 0) { // TODO verify colors
            char guess[10];
            if (sscanf(command, "try %s", guess) == 1)
                handle_try(fdudp, resudp, guess);
            else
                printf("Usage: try C1 C2 C3 C4\n");

        /* show_trials command */
        } else if (strncmp(command, "show_trials", 11) == 0 || strncmp(command, "st", 2) == 0) { 
            handle_show_trials(fdtcp, restcp);

        /* scoreboard command */
        } else if (strncmp(command, "scoreboard", 10) == 0 || strncmp(command, "sb", 2) == 0) {
            handle_scoreboard(fdtcp, restcp);

        /* quit command */
        } else if (strncmp(command, "quit", 4) == 0) {
            char plid[7];
            if (sscanf(command, "quit %6s", plid) == 1)
                handle_quit(fdudp, resudp, plid);
            else
                printf("Usage: quit PLID\n");

        /* exit command */
        } else if (strncmp(command, "exit", 4) == 0) {
            char plid[7];
            if (sscanf(command, "exit %6s", plid) == 1)
                handle_exit(fdudp, resudp, plid);
            else
                printf("Usage: exit PLID\n");
            break;

        /* deboug command */
        } else if (strncmp(command, "debug", 5) == 0) {
            char plid[7], key[10];
            int max_playtime;
            if (sscanf(command, "debug %6s %d %s", plid, &max_playtime, key) == 3)
                handle_debug(fdudp, resudp, plid, max_playtime, key);
            else
                printf("Usage: debug PLID max_playtime C1 C2 C3 C4\n");
        } else {
            printf("Unknown command\n");
        }
    }

    close_connection(fdudp, fdtcp, restcp, resudp);

    return 0;
}