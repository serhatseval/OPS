/**
 * \brief Basic UDP example - intraprocess communication using two sockets.
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

    int client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    printf("--- Before bind() ---\n");
    printf("Client addr: ");
    print_sockname(client_socket);
    printf("\nServer addr: ");
    print_sockname(server_socket);
    printf("\n");

    if (bind(server_socket, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("bind():");
        exit(EXIT_FAILURE);
    }

//    struct sockaddr_in client_addr = addr;
//    client_addr.sin_port += 1;

//    if (bind(client_socket, (struct sockaddr *) &client_addr, sizeof(client_addr)) < 0) {
//        perror("bind():");
//        exit(EXIT_FAILURE);
//    }

    printf("--- After bind() ---\n");
    printf("Client addr: ");
    print_sockname(client_socket);
    printf("\nServer addr: ");
    print_sockname(server_socket);
    printf("\n");

    char msg[] = "hello";
    if (sendto(client_socket, msg, sizeof(msg), 0, (struct sockaddr *) &addr, sizeof(addr)) < 0) { // TODO: Change to server_socket
        perror("sendto()");
        exit(EXIT_FAILURE);
    }

    printf("--- After sendto() ---\n");
    printf("Client addr: ");
    print_sockname(client_socket);
    printf("\nServer addr: ");
    print_sockname(server_socket);
    printf("\n");

    char buff[20]; // TODO: Change to some very small size (i.e. 2 bytes)
    struct sockaddr sender_addr;
    socklen_t sender_addrlen = sizeof(sender_addr);
    ssize_t ret = recvfrom(server_socket, buff, sizeof(buff), 0, (struct sockaddr *) &sender_addr, &sender_addrlen);
    if (ret < 0) {
        perror("sendto()");
        exit(EXIT_FAILURE);
    }

    printf("--- Result ---\n");
    printf("Received datagram of size %ld bytes: '%s'\n", ret, buff);
    printf("From: ");
    print_addr(&sender_addr);
    printf("\n");

    return 0;
}
