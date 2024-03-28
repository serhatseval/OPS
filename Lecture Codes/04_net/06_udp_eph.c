/**
 * \brief UDP ephemeric port example.
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

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(20000);

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

    printf("Server addr before bind: ");
    print_sockname(server_socket);
    printf("\n");

    if (bind(server_socket, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("bind():");
        exit(EXIT_FAILURE);
    }

    printf("Server addr after bind: ");
    print_sockname(server_socket);
    printf("\n");

    return 0;
}
