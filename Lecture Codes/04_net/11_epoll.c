/**
 * \brief Synchronous multiplexing utilizing epoll().
 */

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/epoll.h>
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

    int epoll_fd = epoll_create(100);

    struct epoll_event ev = {};
    ev.data.fd = server_socket;
    ev.events = EPOLLIN;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &ev) < 0) {
        perror("epoll_ctl()");
        exit(EXIT_FAILURE);
    }

    while (1) {

        struct epoll_event events[10];

        printf("Calling epoll_wait()!\n");
        int ret = epoll_wait(epoll_fd, events, 10, -1);
        if (ret < 0) {
            perror("epoll_wait()");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < ret; ++i) {
            int fd = events[i].data.fd;
            if (events[i].events & EPOLLIN) {
                if (fd == server_socket) {
                    struct sockaddr client_addr;
                    socklen_t client_addrlen = sizeof(client_addr);
                    int client = accept(server_socket, &client_addr, &client_addrlen);
                    if (client >= 0) {
                        printf("Accepted new client!\n");
                        set_nonblock(client);
                        ev.data.fd = client;
                        ev.events = EPOLLIN;
                        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client, &ev) < 0) {
                            perror("epoll_ctl()");
                            exit(EXIT_FAILURE);
                        }
                    } else if (errno != EAGAIN) {
                        perror("accept()");
                        exit(EXIT_FAILURE);
                    }
                } else {
                    char buff[16];
                    ssize_t ret = recv(fd, buff, sizeof(buff), 0);
                    printf("recv returned %ld bytes\n", ret);
                    if (ret > 0) {
                        printf("Client sent: '%.*s'\n", (int)ret, buff);
                    } else if (ret == 0) {
                        if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) < 0) {
                            perror("epoll_ctl()");
                            exit(EXIT_FAILURE);
                        }
                        close(fd);
                    } else if (errno != EAGAIN) {
                        perror("recv()");
                        exit(EXIT_FAILURE);
                    }
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
