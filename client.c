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

int initialize_sockets(int* fdtcp, int* fdudp, struct addrinfo **restcp, struct addrinfo **resudp, char* gs_ip, char* gs_port) {
    struct addrinfo hints;
    int errcode;
    *fdtcp = socket(AF_INET, SOCK_STREAM, 0);
    if (*fdtcp == -1) return -1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    errcode = getaddrinfo(gs_ip, gs_port, &hints, restcp);
    if (errcode == -1) return -1;

    *fdudp = socket(AF_INET, SOCK_DGRAM,0);
    if (*fdudp == -1) return -1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    errcode = getaddrinfo(gs_ip, gs_port, &hints, resudp);
    if (errcode == -1) return -1;

    return 0;
}

// TODO maybe (probably) implement select for timeouts

int send_udp(int fdudp, const char* message, struct addrinfo *resudp, char *buffer) {
    printf("Sending message to server: %s\n", message);  // Log the message being sent
    int n, ct = 0;
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fdudp, &fds);
    struct timeval tv;

    while (ct == 0){
        n = sendto(fdudp, message, strlen(message), 0, resudp->ai_addr, resudp->ai_addrlen);
        if (n == -1) return -1;

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        ct = select(fdudp + 1, &fds, NULL, NULL, &tv);
    }

    n = recvfrom(fdudp, buffer, 256*sizeof(char), 0, NULL, NULL);
    if (n == -1) return -1;

    buffer[n] = '\0';  // Null-terminate the buffer
    printf("Received response from server: %s\n", buffer);

    return 0;
}

int send_tcp(int fdtcp, const char* message, struct addrinfo *restcp, char *buffer) {
    int n, ct = 0;
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fdtcp, &fds);
    struct timeval tv;

    if (connect(fdtcp, restcp->ai_addr, restcp->ai_addrlen) == -1) return -1;
    while (ct == 0){
        n = write(fdtcp, message, strlen(message) + 1);
        if (n == -1) return -1;

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        ct = select(fdtcp + 1, &fds, NULL, NULL, &tv);
    }
    sleep(1);
    n = read(fdtcp, buffer, 256*sizeof(char));
    if (n == -1) return -1;

    if (shutdown(fdtcp, SHUT_RDWR) == -1) {
        perror("shutdown");
        return -1;
    }

    return 0;
}

void close_connection(int fdudp, int fdtcp, struct addrinfo *restcp, struct addrinfo *resudp) {
    freeaddrinfo(restcp);
    close(fdtcp);

    freeaddrinfo(resudp);
    close(fdudp);
}
