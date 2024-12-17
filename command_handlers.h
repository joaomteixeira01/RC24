#ifndef COMMAND_HANDLERS_H
#define COMMAND_HANDLERS_H

#include <netdb.h>  // For struct addrinfo

/**
 * Handle the "start" command.
 * Sends a message to the Game Server (GS) to start a new game with the specified PLID and max_playtime.
 * Displays the secret key if the game has started successfully.
 * 
 * @param fdudp        UDP socket file descriptor
 * @param resudp       Address info for the UDP socket
 * @param plid         Player ID (6-digit student number)
 * @param max_playtime Maximum time in seconds to complete the game (cannot exceed 600 seconds)
 * 
 * @return 0 if the game has started successfully, -1 otherwise
 */
int handle_start(int fdudp, struct addrinfo *resudp, char *plid, int max_playtime);

/**
 * Handle the "try" command.
 * Sends a trial message to the Game Server (GS) to check if the provided guess is correct.
 * Displays the server's response, including the number of correct guesses in color and position (nB) and correct colors in incorrect positions (nW).
 * 
 * @param fdudp  UDP socket file descriptor
 * @param resudp Address info for the UDP socket
 * @param guess  The player's guess for the secret key (format: C1 C2 C3 C4)
 * @param nT     Number of trial
 * @param plid   Player ID (6-digit student number)
 */
void handle_try(int fdudp, struct addrinfo *resudp, char *guess, int nT, char *plid);

/**
 * Handle the "show_trials" command.
 * Establishes a TCP session with the Game Server (GS) to request a list of previous trials and their results.
 * Displays the list of trials and the remaining playtime to the player.
 * 
 * @param fdtcp  TCP socket file descriptor
 * @param restcp Address info for the TCP socket
 */
void handle_show_trials(int fdtcp, struct addrinfo *restcp);

/**
 * Handle the "scoreboard" command.
 * Establishes a TCP session with the Game Server (GS) to request the top 10 scores.
 * Displays the scores received from the server.
 * 
 * @param fdtcp  TCP socket file descriptor
 * @param restcp Address info for the TCP socket
 */
void handle_scoreboard(int fdtcp, struct addrinfo *restcp);

/**
 * Handle the "quit" command.
 * Sends a message to the Game Server (GS) to quit the ongoing game for the specified player.
 * 
 * @param fdudp  UDP socket file descriptor
 * @param resudp Address info for the UDP socket
 * @param plid   Player ID (6-digit student number)
 */
void handle_quit(int fdudp, struct addrinfo *resudp, char *plid);

/**
 * Handle the "exit" command.
 * Sends a message to the Game Server (GS) to exit the Player application, terminating any ongoing game.
 * 
 * @param fdudp  UDP socket file descriptor
 * @param resudp Address info for the UDP socket
 * @param plid   Player ID (6-digit student number)
 */
void handle_exit(int fdudp, struct addrinfo *resudp, char *plid);

/**
 * Handle the "debug" command.
 * Sends a message to the Game Server (GS) to start a new game in debug mode, providing a predefined secret key.
 * 
 * @param fdudp        UDP socket file descriptor
 * @param resudp       Address info for the UDP socket
 * @param plid         Player ID (6-digit student number)
 * @param max_playtime Maximum time in seconds to complete the game (cannot exceed 600 seconds)
 * @param key          Predefined secret key for the debug mode (format: C1 C2 C3 C4)
 */
void handle_debug(int fdudp, struct addrinfo *resudp, char *plid, int max_playtime, char *key);

#endif 
