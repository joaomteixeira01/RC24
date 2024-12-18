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

int handle_start(int fdudp, struct addrinfo *resudp, char *plid, int max_playtime) {
    char message[256];
    char buffer[256];

    if (max_playtime > 600 || max_playtime < 1) {
        printf("Error: Invalid max playtime\n");
        return -1;
    }


    // Format the SNG request message
    snprintf(message, sizeof(message), "SNG %s %03d\n", plid, max_playtime);
    
    if (send_udp(fdudp, message, resudp, buffer) == -1) {
        printf("Error: Failed to send start command\n");
        return -1;
    } 

    // Check the response from the Game Server
    if (strncmp(buffer, "RSG", 3) == 0 && strncmp(buffer + 4, "OK", 2) == 0) {
        // The response starts with "RSG OK", so the game has started successfully
        printf("New game started (max %d sec)\n", max_playtime);
        return 0;
    } 
    if(strncmp(buffer, "RSG", 3) == 0 && strncmp(buffer + 4, "NOK", 3) == 0) {
        // If the response is "RSG NOK", the game could not be started
        printf("Error: Game could not be started (is the player already in a game?)\n");
        return -1;
    } 
    if(strncmp(buffer, "RSG", 3) == 0 && strncmp(buffer + 4, "ERR", 3) == 0) {
        // If the response is "RSG ERR", there is something wrong with the given parameters
        printf("Error: Invalid Input\n");
        return -1;
    }
    // Unexpected response from the server
    printf("Error: Unexpected response from the server\n");
    return -1;
}

int handle_try(int fdudp, struct addrinfo *resudp, char *guess, int nT, char *plid) { // TODO add return values for error handling!
    char message[256];
    char buffer[256];

    // Format the TRY request message
    snprintf(message, sizeof(message), "TRY %s %s %d\n",plid, guess, nT);

    // Check the response from the Game Server
    if (send_udp(fdudp, message, resudp, buffer) == -1) {
        printf("Error: Failed to send TRY command\n");
        return -1;

    } else if (strncmp(buffer, "RTR", 3) == 0 && strncmp(buffer + 4, "OK", 2) == 0) {
        int nTn, nB, nW;

        // The response starts with "RTR OK", so the guess was correctly received
        if (sscanf(buffer + 7, "%d %d %d", &nTn, &nB, &nW) == 3) { 
            if (nTn == nT - 1) send_udp(fdudp, message, resudp, buffer); // Resend (might be wrong) FIXME
            printf("Correct guesses in color and position (nB): %d\n", nB);
            printf("Correct colors in incorrect positions (nW): %d\n", nW);
            return 0;
        } else {
            printf("Error: Failed to parse server response\n");
            return -1;
        }

    } else if (strncmp(buffer, "RTR", 3) == 0 && strncmp(buffer + 4, "NOK", 3) == 0) { 
        // If the response is "RTR NOK", the guess could not be processed
        printf("Error: Cannot submit guess (is %s in a game?)\n", plid);
        return -1;

    } else if (strncmp(buffer, "RTR", 3) == 0 && strncmp(buffer + 4, "DUP", 3) == 0) {
        printf("Error: Duplicate trial\n");
        return -1;

    } else if (strncmp(buffer, "RTR", 3) == 0 && strncmp(buffer + 4, "INV", 3) == 0) {
        printf("Error: Invalid trial (internal error)\n");
        return -1;

    } else if (strncmp(buffer, "RTR", 3) == 0 && strncmp(buffer + 4, "ENT", 3) == 0) {
        printf("Game Over (max trials reached)\nCorrect Answer: %s\n", buffer + 7);
        return 1;

    } else if (strncmp(buffer, "RTR", 3) == 0 && strncmp(buffer + 4, "ETM", 3) == 0) {
        printf("Game Over (time is up)\nCorrect Answer: %s\n", buffer + 7);
        return 1;

    } else if (strncmp(buffer, "RTR", 3) == 0 && strncmp(buffer + 4, "ERR", 3) == 0) {
        printf("Error: Server error\n");
        return -1;

    } else {
        printf("Error: Unexpected response from the server\n");
        return -1;
    } 
}

void handle_show_trials(int fdtcp, struct addrinfo *restcp, char* plid) { // TODO finish - output to text file, format for terminal output
    char message[256];
    char buffer[1024];
    char fname[25];
    char fsize[5];

    snprintf(message, sizeof(message), "STR %s\n", plid);

    if (send_tcp(fdtcp, message, restcp, buffer) == -1) {
        printf("Error: Failed to fetch trials\n");
        return;
    } else if (strncmp(buffer, "RST", 3) == 0 && (strncmp(buffer + 4, "ACT", 3) == 0 || strncmp(buffer + 4, "FIN", 3) == 0)) {
        if (strncmp(buffer + 4, "ACT", 3) == 0) {
            printf("Current game:\n");
        } else {
            printf("Last game:\n");
        }
        sscanf(buffer + 8, "%s %s", fname, fsize);
        FILE *file = fopen(fname, "w");
        if (file == NULL) {
            printf("Error: Could not open file %s for writing\n", fname);
            return;
        }
        write(1, buffer + strlen(fname) + strlen(fsize) + 10, atoi(fsize));
        fwrite(buffer + strlen(fname) + strlen(fsize) + 10, 1, atoi(fsize), file);
        fclose(file);
        printf("Trials saved to %s\n", fname);
    } else if (strncmp(buffer, "RST", 3) == 0 && strncmp(buffer + 4, "NOK", 3) == 0) {
        printf("Error: No game history\n");
    } else if (strncmp(buffer, "RST", 3) == 0 && strncmp(buffer + 4, "ERR", 3) == 0) {
        printf("Error: Server error\n");
    } else {
        printf("Error: Unexpected response from the server\n");
    }
}

void handle_scoreboard(int fdtcp, struct addrinfo *restcp) { // TODO finish - idek figure it out later
    char message[] = "SSB\n";
    char buffer[4096];
    char fname[25];
    char fsize[5];

    if (send_tcp(fdtcp, message, restcp, buffer) == -1) {
        printf("Error: Failed to fetch scoreboard\n");
        return;
    } else if (strncmp(buffer, "RSS", 3) == 0 && strncmp(buffer + 4, "OK", 2) == 0) {
        sscanf(buffer + 7, "%s %s", fname, fsize);
        FILE *file = fopen(fname, "w");
        if (file == NULL) {
            printf("Error: Could not open file %s for writing\n", fname);
            return;
        }
        write(1, buffer + strlen(fname) + strlen(fsize) + 10, atoi(fsize));
        fwrite(buffer + strlen(fname) + strlen(fsize) + 10, 1, atoi(fsize), file);
        fclose(file);
        printf("Scoreboard saved to %s\n", fname);

    } else if (strncmp(buffer, "RSS", 3) == 0 && strncmp(buffer + 4, "EMPTY", 5) == 0) {
        printf("Error: No scoreboard available\n");
    } else {
        printf("Error: Unexpected response from the server\n");
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

int handle_debug(int fdudp, struct addrinfo *resudp, char *plid, int max_playtime, char *key) {
    char message[256];
    char buffer[256];

    if (max_playtime > 600 || max_playtime < 1) {
        printf("Error: Invalid max playtime\n");
        return -1;
    }


    // Format the DBG request message
    snprintf(message, sizeof(message), "DBG %s %03d %s\n", plid, max_playtime, key);

    if (send_udp(fdudp, message, resudp, buffer) == -1) {
        printf("Error: Failed to send debug command\n");
        return -1;
    }

    // Check the response from the Game Server
    if (strncmp(buffer, "RDB", 3) == 0 && strncmp(buffer + 4, "OK", 2) == 0) {
        // The response starts with "RDG OK", so the game has started successfully
        printf("New game started in debug mode (max %d sec)\n", max_playtime);
        return 0;
    }

    if (strncmp(buffer, "RDB", 3) == 0 && strncmp(buffer + 4, "NOK", 3) == 0) {
        // If the response is "RDG NOK", the game could not be started
        printf("Error: Game could not be started (is the player already in a game?)\n");
        return -1;
    }

    if (strncmp(buffer, "RDB", 3) == 0 && strncmp(buffer + 4, "ERR", 3) == 0) {
        // If the response is "RDG ERR", there is something wrong with the given parameters
        printf("Error: Invalid Input\n");
        return -1;
    }

    // Unexpected response from the server
    printf("Error: Unexpected response from the server\n");
    return -1;
}