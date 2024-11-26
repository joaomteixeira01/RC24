#include <stdio.h>        // For printf, snprintf
#include <stdlib.h>       // For exit
#include <string.h>       // For strncmp, memset
#include <netdb.h>        // For struct addrinfo and networking functions

#include "client.h"       // For send_udp, send_tcp, and shared socket utilities
#include "command_handlers.h"

void handle_start(int fdudp, struct addrinfo *resudp, char *plid, int max_playtime) {
    char message[256];
    char buffer[256];

    if (max_playtime > 600) {
        printf("Error: max_playtime cannot exceed 600 seconds.\n");
        return;
    }

    // Format the START request message
    snprintf(message, sizeof(message), "START %s %d\n", plid, max_playtime);
    
    if (send_udp(fdudp, message, resudp, buffer) == -1) {
        printf("Error: Failed to send START command\n");
    } else {
        printf("Server response: %s\n", buffer);
    }

    // Check the response from the Game Server
    if (strncmp(buffer, "OK", 2) == 0) {
        // The response starts with "OK", followed by the generated secret key
        printf("Game started successfully!\n");
        printf("Secret key: %s\n", buffer + 3); 
    } else {
        // If the response is not "OK", display the error message
        printf("Server response: %s\n", buffer);
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
    } else {
        printf("Server response: %s\n", buffer);
    }
}

void handle_show_trials(int fdtcp, struct addrinfo *restcp) {
    char message[] = "SHOW_TRIALS\n";
    char buffer[1024];

    if (send_tcp(fdtcp, message, restcp, buffer) == -1) {
        printf("Error: Failed to fetch trials\n");
    } else {
        printf("Trials:\n%s\n", buffer);
    }
}

void handle_scoreboard(int fdtcp, struct addrinfo *restcp) {
    // Implementar
}

void handle_quit(int fdudp, struct addrinfo *resudp, char *plid) {
    //Implementar
}

void handle_exit(int fdudp, struct addrinfo *resudp, char *plid) {
    //Implementar
}

void handle_debug(int fdudp, struct addrinfo *resudp, char *plid, int max_playtime, char *key) {
    //Implementar
}