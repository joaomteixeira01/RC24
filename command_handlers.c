/*
 * This file implements the handlers for various client commands to interact 
 * with the Game Server using UDP and TCP protocols. Each function corresponds 
 * to a specific command and manages message formatting, communication, and 
 * response processing.
 *
 * Implemented Commands:
 * - handle_start: Starts a new game session.
 * - handle_try: Submits a guess for the game.
 * - handle_show_trials: Retrieves the list of previous trials via TCP.
 * - handle_scoreboard: Fetches the game's scoreboard via TCP.
 */

#include <stdio.h>        
#include <stdlib.h>       
#include <string.h>       
#include <netdb.h>        

#include "client.h"       
#include "command_handlers.h"

void handle_start(int fdudp, struct addrinfo *resudp, char *plid, int max_playtime) {
    char message[256];
    char buffer[256];

    if (max_playtime > 600) {
        printf("Error: max_playtime cannot exceed 600 seconds.\n");
        return;
    }

    // Format the START request message
    snprintf(message, sizeof(message), "SNG %s %03d\n", plid, max_playtime);
    
    if (send_udp(fdudp, message, resudp, buffer) == -1) {
        printf("Error: Failed to send start command\n");
    } else {
        printf("Server response: %s\n", buffer);
    }

    // Check the response from the Game Server
    if (strncmp(buffer, "RSG", 3) == 0 && strncmp(buffer + 4, "OK", 2) == 0) {
        // The response starts with "RSG OK", so the game has started successfully
        printf("Game started successfully!\n"); // TODO maybe remove this? idk see if it's needed
    } else if(strncmp(buffer, "RSG", 3) == 0 && strncmp(buffer + 4, "NOK", 3) == 0) {
        // If the response is "RSG NOK", the game could not be started
        printf("Server response: %s\n", buffer);
    } else {
        // Unexpected response from the server
        printf("Error: Unexpected response from the server\n");
    }
}

void handle_try(int fdudp, struct addrinfo *resudp, char *guess) {
    char message[256];
    char buffer[256];

    // Format the TRY request message
    snprintf(message, sizeof(message), "TRY %s\n", guess);

    // Check the response from the Game Server
    if (send_udp(fdudp, message, resudp, buffer) == -1) {
        printf("Error: Failed to send TRY command\n");
    } else if (strncmp(buffer, "RTR", 3) == 0 && strncmp(buffer + 4, "OK", 2) == 0) {
        int nT, nB, nW;
        // The response starts with "RTR OK", so the guess was correctly received
        if (sscanf(buffer + 7, "%d %d %d", &nT, &nB, &nW) == 3) { // TODO find out where to use nT
            printf("Correct guesses in color and position (nB): %d\n", nB);
            printf("Correct colors in incorrect positions (nW): %d\n", nW);
        } else {
            printf("Error: Failed to parse server response\n");
        }
    } else if (strncmp(buffer, "RTR", 3) == 0 && strncmp(buffer + 4, "NOK", 3) == 0) { // TODO Handle Other Responses
        // If the response is "RTR NOK", the guess could not be processed
        printf("Server response: %s\n", buffer);
    } else {
        // Unexpected response from the server
        printf("Error: Unexpected response from the server\n");
    }
}

void handle_show_trials(int fdtcp, struct addrinfo *restcp) { // TODO finish - output to text file, format for terminal output
    char message[] = "STR\n";
    char buffer[1024];

    if (send_tcp(fdtcp, message, restcp, buffer) == -1) {
        printf("Error: Failed to fetch trials\n");
    } else {
        printf("Trials:\n%s\n", buffer);
    }
}

void handle_scoreboard(int fdtcp, struct addrinfo *restcp) { // TODO finish - idek figure it out later
    char message[] = "SSB\n";
    char buffer[1024];

    if (send_tcp(fdtcp, message, restcp, buffer) == -1) {
        printf("Error: Failed to fetch scoreboard\n");
    } else {
        printf("Scoreboard:\n%s\n", buffer);
    }
}

void handle_quit(int fdudp, struct addrinfo *resudp, char *plid) {
    char message[256];
    char buffer[256];
    snprintf(message, sizeof(message), "QUT %s\n", plid);

    if (send_udp(fdudp, message, resudp, buffer) == -1) {
        printf("Error: Failed to send exit command\n");
    } else if (strncmp(buffer, "RQT", 3) == 0 && strncmp(buffer + 4, "OK", 2) == 0) {
        // The response starts with "RQT OK", Game Over and Correct Answer revealed
        printf("Game Over\nCorrect Answer: %s\n", buffer + 7);
    } else if (strncmp(buffer, "RQT", 3) == 0 && strncmp(buffer + 4, "NOK", 3) == 0) {
        // The response starts with "RQT NOK", the player is not in a game
        printf("Error: %s is not in a game.\n", plid);
    } else if (strncmp(buffer, "RQT", 3) == 0 && strncmp(buffer + 4, "ERR", 3) == 0) {
        printf("Error: Server error\n");
    } else {
        printf("Error: Unexpected response from the server\n");
    }

}

void handle_debug(int fdudp, struct addrinfo *resudp, char *plid, int max_playtime, char *key) {
    //Implementar
}