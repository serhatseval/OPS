/**
 * \brief Basic networking example - send single UDP packet to given destination.
 *
 * \todo Invoke with various hosts/ports
 * \todo Send packet to localhost
 * \todo Send packet to netcat `nc -ul <port>`
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>

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

int main(int argc, const char *argv[]) {

    if (argc != 3) {
        fprintf(stderr, "USAGE: ./01_base host port\n");
        exit(EXIT_FAILURE);
    }

    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr = make_address(argv[1], argv[2]);

    char msg[] = "hello";
    // char msg[20000] = {};
    if (sendto(s, msg, sizeof(msg), 0, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("sendto()");
        exit(EXIT_FAILURE);
    }

    return 0;
}
