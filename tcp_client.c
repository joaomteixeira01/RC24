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
    struct addrinfo hints, *res;
    char buffer[128];

    // Criação do socket TCP
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        exit(1);
    }

    // Configuração do addrinfo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;          // IPv4
    hints.ai_socktype = SOCK_STREAM;    // Socket TCP

    // Obter endereço do servidor
    //errcode = getaddrinfo("tejo.tecnico.ulisboa.pt", PORT, &hints, &res);
    errcode = getaddrinfo("localhost", PORT, &hints, &res); 
    if (errcode != 0) {
        perror("getaddrinfo");
        exit(1);
    }

    // Estabelecer conexão com o servidor
    if (connect(fd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("connect");
        exit(1);
    }

    printf("Connected to server.\n");

    // Enviar mensagem ao servidor
    n = write(fd, "Hello!\n", 7);
    if (n == -1) {
        perror("write");
        exit(1);
    }

    // Receber resposta do servidor
    n = read(fd, buffer, 128);
    if (n == -1) {
        perror("read");
        exit(1);
    }

    // Exibir a resposta
    write(1, "echo: ", 6);
    write(1, buffer, n);

    // Limpeza e encerramento
    freeaddrinfo(res);
    close(fd);

    return 0;
}
