/**
 * \brief Network address conversion example.
 * \todo Try to invoke with address 127.0.0.1
 * \todo Try to invoke with address 255.255.255.255
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

    in_addr_t addr = inet_addr(host);  // TODO: Try using inet_aton (non-POSIX)
    if (addr == INADDR_NONE) {
        fprintf(stderr, "Invalid address: %s\n", host);
        return 1;
    }

    struct in_addr address = {addr};

    printf("Hex: %08x\n", addr);  // TODO: How this printout would look like on a big-endian machine? Try htonl() here.
    const char* baddr = (const char*)&addr;
    printf("Bytes: [%02x, %02x, %02x, %02x]\n", baddr[0], baddr[1], baddr[2], baddr[3]);

    const char *addr_txt = inet_ntoa(address);
    printf("Text: %s\n", addr_txt);

    return 0;
}
