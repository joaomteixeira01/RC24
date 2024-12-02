/*
 * This file contains functions to initialize and manage TCP and UDP socket connections 
 * with a remote server. It includes the following functions:
 *
 * - int initialize_sockets(int* fdudp, int* fdtcp, struct addrinfo *restcp, struct addrinfo *resudp):
 *   Initializes TCP and UDP sockets and establishes a connection to the specified server.
 *
 * - int send_udp(int fdudp, const char* message, struct addrinfo *resudp, char *buffer):
 *   Sends a message to the server using UDP and receives the response.
 *
 * - int send_tcp(int fdtcp, const char* message, struct addrinfo *restcp, char *buffer):
 *   Sends a message to the server using TCP and receives the response.
 *
 * - void close_connection(int fdudp, int fdtcp, struct addrinfo *restcp, struct addrinfo *resudp):
 *   Closes the TCP and UDP sockets and frees associated resources.
 */

#include "client.h"

int initialize_sockets(int* fdudp, int* fdtcp, struct addrinfo *restcp, struct addrinfo *resudp) {
    struct addrinfo hints, *res;
    int errcode;
    fdtcp = socket(AF_INET, SOCK_STREAM, 0);
    if (fdtcp == -1) return -1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    errcode = getaddrinfo("tejo.tecnico.ulisboa.pt", PORT, &hints, &restcp);
    if (errcode == -1) return -1;

    if(connect(*fdtcp, restcp->ai_addr, restcp->ai_addrlen) == -1) 
        return -1;
    
    fdudp = socket(AF_INET, SOCK_DGRAM,0);
    if (fdudp == -1) return -1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    errcode = getaddrinfo("tejo.tecnico.ulisboa.pt", PORT, &hints, &resudp);
    if (errcode == -1) return -1;

    return 0;
}

int send_udp(int fdudp, const char* message, struct addrinfo *resudp, char *buffer) {
    int n;

    n = sendto(fdudp, message, strlen(message) + 1, 0, resudp->ai_addr, resudp->ai_addrlen);
    if (n == -1) return -1;

    n = recvfrom(fdudp, buffer, sizeof(buffer), 0, NULL, NULL);
    if (n == -1) return -1;

    return 0;
}

int send_tcp(int fdtcp, const char* message, struct addrinfo *restcp, char *buffer) {
    int n;

    n = write(fdtcp, message, strlen(message) + 1);
    if (n == -1) return -1;

    n = read(fdtcp, buffer, sizeof(buffer));
    if (n == -1) return -1;

    return 0;
}

void close_connection(int fdudp, int fdtcp, struct addrinfo *restcp, struct addrinfo *resudp) {
    freeaddrinfo(restcp);
    close(fdtcp);

    freeaddrinfo(resudp);
    close(fdudp);
}
