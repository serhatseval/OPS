/**
 * \brief Synchronous multiplexing utilizing select().
 */

#include <errno.h>
#include <fcntl.h>
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

#define MAX_CLIENTS 3

int set_nonblock(int desc) {
    int oldflags = fcntl(desc, F_GETFL, 0);
    if (oldflags == -1)
        return -1;
    oldflags |= O_NONBLOCK;
    return fcntl(desc, F_SETFL, oldflags);
}

void do_server(int server_socket) {

    int clients[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; ++i)
        clients[i] = -1;

    while (1) {
        fd_set rdset;
        FD_ZERO(&rdset);
        FD_SET(server_socket, &rdset);

        int maxfd = server_socket;
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clients[i] != -1) {
                FD_SET(clients[i], &rdset);
                if (clients[i] > maxfd)
                    maxfd = clients[i];
            }
        }

        printf("Calling select()!\n");
        int ret = select(maxfd + 1, &rdset, NULL, NULL, NULL);
        if (ret < 0) {
            perror("select()");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(server_socket, &rdset)) {
            struct sockaddr client_addr;
            socklen_t client_addrlen = sizeof(client_addr);
            int client = accept(server_socket, &client_addr, &client_addrlen);

            if (client >= 0) {
                printf("Accepted new client!\n");
                set_nonblock(client);
                for (int i = 0; i < MAX_CLIENTS; ++i) {
                    if (clients[i] == -1) {
                        clients[i] = client;
                        break;
                    } else if (i == MAX_CLIENTS - 1) {
                        char neat_reply[] = "No space for you :(\n";
                        send(client, neat_reply, sizeof(neat_reply), 0);
                        close(client);
                    }
                }
            } else if (errno != EAGAIN) {
                perror("accept()");
                exit(EXIT_FAILURE);
            }
        }

        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (FD_ISSET(clients[i], &rdset)) {
                char buff[16];
                ssize_t ret = recv(clients[i], buff, sizeof(buff), 0);
                printf("recv returned %ld bytes\n", ret);
                if (ret > 0) {
                    printf("Client sent: '%.*s'\n", (int)ret, buff);
                } else if (ret == 0) {
                    close(clients[i]);
                    clients[i] = -1;
                } else if (errno != EAGAIN) {
                    perror("recv()");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
}

int main(int argc, const char *argv[]) {

    if (argc != 3) {
        fprintf(stderr, "USAGE: %s host port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr = make_address(argv[1], argv[2]);
    int server_socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
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

    do_server(server_socket);

    close(server_socket);
    return 0;
}
