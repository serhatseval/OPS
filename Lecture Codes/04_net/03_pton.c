/**
 * \brief Network address conversion example.
 * \todo Try to invoke with address 127.0.0.1
 * \todo Try to invoke with address 255.255.255.255
 * \todo Try changing this example to AF_INET6
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

int main(int argc, const char *argv[]) {

    if (argc != 2) {
        fprintf(stderr, "USAGE: %s host\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *host = argv[1];

    struct in_addr addr;

    int ret = inet_pton(AF_INET, host, &addr);
    if (ret != 1) { // Note 1 here...
        perror("inet_pton()");
        return 1;
    }

    printf("Hex: %08x\n", addr.s_addr);

    char txt[INET_ADDRSTRLEN];
    const char *ret2 = inet_ntop(AF_INET, &addr, txt, sizeof(txt));
    if (ret2 == NULL) {
        perror("inet_ntop()");
        return 1;
    }

    printf("Text: %s\n", txt);

    return 0;
}
