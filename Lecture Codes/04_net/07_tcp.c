/**
 * \brief Basic TCP example - intraprocess communication from client to server.
 */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

struct sockaddr_in make_address(const char *address, const char *port) {
    int ret;
    struct sockaddr_in addr;
    struct addrinfo *result;
    struct addrinfo hints = {};
    hints.ai_family = AF_INET;
    if ((ret = getaddrinfo(address, port, &hints, &result))) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        exit(EXIT_FAILURE);
    }
    addr = *(struct sockaddr_in *) (result->ai_addr);
    freeaddrinfo(result);
    return addr;
}

void print_addr_in(struct sockaddr_in *addr) {
    char txt[128];
    const char *ret = inet_ntop(AF_INET, &addr->sin_addr, txt, sizeof(txt));
    if (ret == NULL) {
        perror("inet_ntop()");
    } else {
        printf("%s:%d", txt, htons(addr->sin_port));
    }
}

void print_addr_in6(struct sockaddr_in6 *addr) {
    char txt[128];
    const char *ret = inet_ntop(AF_INET6, &addr->sin6_addr, txt, sizeof(txt));
    if (ret == NULL) {
        perror("inet_ntop()");
    } else {
        printf("%s:%d", txt, htons(addr->sin6_port));
    }
}

void print_addr(struct sockaddr *addr) {
    switch (addr->sa_family) {
        case AF_INET:
            return print_addr_in((struct sockaddr_in *) addr);
        case AF_INET6:
            return print_addr_in6((struct sockaddr_in6 *) addr);
        default:
            printf("Unknown address family\n");
    }
}

void print_sockname(int socket) {
    struct sockaddr addr;
    socklen_t addrlen = sizeof(addr);
    if (getsockname(socket, &addr, &addrlen) < 0) {
        perror("getsockname()");
        exit(EXIT_FAILURE);
    }
    print_addr(&addr);
}

int main(int argc, const char *argv[]) {

    if (argc != 3) {
        fprintf(stderr, "USAGE: %s host port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr = make_address(argv[1], argv[2]);

    // Server socket configuration

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    if (bind(server_socket, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 3) < 0) {
        perror("listen()");
        exit(EXIT_FAILURE);
    }

    printf("Server sockname: ");
    print_sockname(server_socket);
    printf("\n");

    // Client socket creation and establishing connection

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    printf("connect? >");
    fflush(stdout);
    getchar();

    if (connect(client_socket, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("connect()");
        exit(EXIT_FAILURE);
    }

    printf("Connected client sockname: ");
    print_sockname(client_socket);
    printf("\n");

    printf("accept? >");
    fflush(stdout);
    getchar();

    struct sockaddr client_addr;
    socklen_t client_addrlen = sizeof(client_addr);
    int accepted_client_socket = accept(server_socket, &client_addr, &client_addrlen);
    if (accepted_client_socket < 0) {
        perror("accept()");
        exit(EXIT_FAILURE);
    }

    printf("Accepted client sockname: ");
    print_sockname(accepted_client_socket);
    printf("\n");

    printf("send? >");
    fflush(stdout);
    getchar();

    char msg[] = "hello";
    if (send(client_socket, msg, sizeof(msg), 0) < 0) { // TODO: Change to server_socket
        perror("sendto()");
        exit(EXIT_FAILURE);
    }

    printf("receive? >");
    fflush(stdout);
    getchar();

    char buff[20]; // TODO: Change to some very small size (i.e. 2 bytes)
    ssize_t ret = recv(accepted_client_socket, buff, sizeof(buff), 0);
    if (ret < 0) {
        perror("sendto()");
        exit(EXIT_FAILURE);
    }

    printf("Received data of size %ld bytes: '%s'\n", ret, buff);

    printf("close client? >");
    fflush(stdout);
    getchar();

    close(client_socket);

    printf("close server? >");
    fflush(stdout);
    getchar();

    close(accepted_client_socket);
    close(server_socket);

    return 0;
}
