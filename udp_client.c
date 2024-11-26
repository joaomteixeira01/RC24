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
    int fd, errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[128];

    // Criação do socket UDP
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        perror("socket");
        exit(1);
    }

    // Configuração do addrinfo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;          // IPv4
    hints.ai_socktype = SOCK_DGRAM;     // Socket UDP

    // Obter endereço do servidor
    //errcode = getaddrinfo("tejo.tecnico.ulisboa.pt", PORT, &hints, &res);
    errcode = getaddrinfo("localhost", PORT, &hints, &res);
    if (errcode != 0) {
        perror("getaddrinfo");
        exit(1);
    }

    // Enviar mensagem para o servidor
    n = sendto(fd, "Hello!\n", 7, 0, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        perror("sendto");
        exit(1);
    }

    // Receber resposta do servidor
    addrlen = sizeof(addr);
    printf("Message sent to server.\n");
    n = recvfrom(fd, buffer, 128, 0, (struct sockaddr *)&addr, &addrlen);
    if (n == -1) {
        perror("recvfrom");
        exit(1);
    }

    printf("Response from server: %.*s\n", (int)n, buffer);

    // Escrever resposta recebida
    write(1, "echo: ", 6);
    write(1, buffer, n);

    // Limpeza e encerramento
    freeaddrinfo(res);
    close(fd);

    return 0;
}

