/**
 * \brief TCP example - child process sends data to the parent process.
 */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/wait.h>
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

void do_child(struct sockaddr_in addr) {

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    if (connect(client_socket, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("connect()");
        exit(EXIT_FAILURE);
    }

    while (1) // TODO: comment it out
    {
        char msg[] = "hello my friend - this is my message for you";
        printf("sending message of size %ld\n", sizeof(msg));
        if (send(client_socket, msg, sizeof(msg), 0) < 0) {
            perror("send()");
            exit(EXIT_FAILURE);
        }
        sleep(1);
    }

    close(client_socket);
}

void do_parent(struct sockaddr_in addr) {
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

    struct sockaddr client_addr;
    socklen_t client_addrlen = sizeof(client_addr);
    int accepted_client_socket = accept(server_socket, &client_addr, &client_addrlen);
    if (accepted_client_socket < 0) {
        perror("accept()");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        char buff[16];
        ssize_t ret = recv(accepted_client_socket, buff, sizeof(buff), 0);
        printf("recv returned %ld bytes\n", ret);
        if (ret < 0) {
            perror("recv()");
            exit(EXIT_FAILURE);
        }
        // sleep(1); // TODO: Do nothing instead of receiving (also comment out sleep in child)
    }

    close(accepted_client_socket);
    close(server_socket);
}

int main(int argc, const char *argv[]) {

    if (argc != 3) {
        fprintf(stderr, "USAGE: %s host port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr = make_address(argv[1], argv[2]);

    switch (fork()) {
        case -1:
            perror("fork()");
            exit(EXIT_FAILURE);
        case 0:
            // child
            do_child(addr);
            exit(EXIT_SUCCESS);
        default:
            // parent
            do_parent(addr);
            while (wait(NULL) > 0);
            exit(EXIT_SUCCESS);
    }
}
