#ifndef CLIENT_H
#define CLIENT_H

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT "58053"

int initialize_sockets(int* fdtcp, int* fdudp, struct addrinfo **restcp, struct addrinfo **resudp, char* gs_ip, char* gs_port);
int send_udp(int fdudp, const char* message, struct addrinfo *resudp, char *buffer);
int send_tcp(int* fdtcp, const char* message, struct addrinfo *restcp, char *buffer);
void close_connection(int fdudp, int fdtcp, struct addrinfo *restcp, struct addrinfo *resudp);

#endif