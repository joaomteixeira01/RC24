#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT "58001" // Porta do servidor

int main() {
    int fd, newfd, errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[128];

    printf("Starting TCP Server...\n");

    // Criação do socket TCP
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        exit(1);
    }

    printf("Socket created.\n");

    // Configuração do addrinfo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;          // IPv4
    hints.ai_socktype = SOCK_STREAM;   // Socket TCP
    hints.ai_flags = AI_PASSIVE;       // Aceitar conexões de qualquer endereço

    // Obter endereço para bind
    errcode = getaddrinfo(NULL, PORT, &hints, &res);
    if (errcode != 0) {
        perror("getaddrinfo");
        exit(1);
    }

    // Associar o socket ao endereço
    if (bind(fd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("bind");
        exit(1);
    }

    printf("Socket bound to port.\n");

    // Escutar conexões
    if (listen(fd, 5) == -1) {
        perror("listen");
        exit(1);
    }

    printf("Server is listening for connections...\n");

    while (1) {
        // Aceitar conexão de um cliente
        addrlen = sizeof(addr);
        newfd = accept(fd, (struct sockaddr *)&addr, &addrlen);
        if (newfd == -1) {
            perror("accept");
            exit(1);
        }

        printf("Client connected.\n");

        // Ler mensagem do cliente
        n = read(newfd, buffer, 128);
        if (n == -1) {
            perror("read");
            exit(1);
        }

        printf("Message received: %.*s\n", (int)n, buffer);

        // Enviar resposta ao cliente
        n = write(newfd, buffer, n);
        if (n == -1) {
            perror("write");
            exit(1);
        }

        printf("Message echoed back to client.\n");

        // Fechar conexão com o cliente
        close(newfd);
    }

    // Limpeza e encerramento
    freeaddrinfo(res);
    close(fd);

    return 0;
}
