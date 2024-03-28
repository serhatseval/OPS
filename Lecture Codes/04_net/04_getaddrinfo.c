/**
 * \brief Network address resolution example (DNS).
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

const char *socktype_string(int sock_type) {
    switch (sock_type) {
        case SOCK_STREAM:
            return "SOCK_STREAM";
        case SOCK_DGRAM:
            return "SOCK_DGRAM";
        case SOCK_RAW:
            return "SOCK_RAW";
        case SOCK_RDM:
            return "SOCK_RDM";
        case SOCK_SEQPACKET:
            return "SOCK_SEQPACKET";
        default:
            return "SOCK_???";
    }
}

const char *af_string(int address_family) {
    switch (address_family) {
        case AF_INET:
            return "AF_INET";
        case AF_INET6:
            return "AF_INET6";
        case AF_LOCAL:
            return "AF_LOCAL";
        default:
            return "AF_???";
    }
}

int main(int argc, const char *argv[]) {

    if (argc < 2) {
        fprintf(stderr, "USAGE: %s host [service]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *host = argv[1];
    const char *service = argc > 2 ? argv[2] : NULL;

    int ret;
    struct addrinfo *result;
    struct addrinfo hints = {};
    // hints.ai_family = AF_INET; // TODO: Uncomment
    if ((ret = getaddrinfo(host, service, &hints, &result))) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        exit(EXIT_FAILURE);
    }

    char txt[512];

    // Iterate over retuner linked list
    for (struct addrinfo *it = result; it != NULL; it = it->ai_next) {

        switch (it->ai_family) {
            case AF_INET: {
                struct sockaddr_in *addr = (struct sockaddr_in *) it->ai_addr;
                const char *ret = inet_ntop(it->ai_family, &addr->sin_addr, txt, sizeof(txt));
                if (ret == NULL) {
                    perror("inet_ntop()");
                    return 1;
                }
                printf("%s: %s:%d %s\n", af_string(it->ai_family), txt, ntohs(addr->sin_port), socktype_string(it->ai_socktype));
                break;
            }
            case AF_INET6: {
                struct sockaddr_in6 *addr = (struct sockaddr_in6 *) it->ai_addr;
                const char *ret = inet_ntop(it->ai_family, &addr->sin6_addr, txt, sizeof(txt));
                if (ret == NULL) {
                    perror("inet_ntop()");
                    return 1;
                }
                printf("%s: %s:%d %s\n", af_string(it->ai_family), txt, ntohs(addr->sin6_port), socktype_string(it->ai_socktype));
                break;
            }
            default:
                printf("Unknown AF: %d\n", it->ai_family);
                break;
        }

    }

    freeaddrinfo(result);

    return 0;
}
