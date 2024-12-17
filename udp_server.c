#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT "58053" // Porta do servidor

int main() {
    int fd, errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[128];

    printf("Starting UDP Server...\n");

    // Criação do socket UDP
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        perror("socket");
        exit(1);
    }

    printf("Socket created.\n");

    // Configuração do addrinfo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;          // IPv4
    hints.ai_socktype = SOCK_DGRAM;     // Socket UDP
    hints.ai_flags = AI_PASSIVE;        // Aceitar conexões de qualquer endereço

    // Obter endereço para bind
    errcode = getaddrinfo(NULL, PORT, &hints, &res);
    if (errcode != 0) {
        perror("getaddrinfo");
        exit(1);
    }

    // Associar o socket ao endereço
    n = bind(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        perror("bind");
        exit(1);
    }

    printf("Server is ready and waiting for messages...\n");

    while (1) {
        // Receber mensagem do cliente
        addrlen = sizeof(addr);
        n = recvfrom(fd, buffer, 128, 0, (struct sockaddr *)&addr, &addrlen);
        if (n == -1) {
            perror("recvfrom");
            exit(1);
        }

        printf("Message received: %.*s\n", (int)n, buffer);

        // Enviar resposta para o cliente
        strcpy(buffer, "RTR OK 1 2 1 R Y O G\n");
        n = sendto(fd, buffer, strlen(buffer), 0, (struct sockaddr *)&addr, addrlen);
        if (n == -1) {
            perror("sendto");
            exit(1);
        }

        printf("Message echoed back to client.\n");
    }

    // Limpeza e encerramento
    freeaddrinfo(res);
    close(fd);

    return 0;
}
