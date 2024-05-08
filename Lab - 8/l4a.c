#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(expression)             \
    (__extension__({                               \
        long int __result;                         \
        do                                         \
            __result = (long int)(expression);     \
        while (__result == -1L && errno == EINTR); \
        __result;                                  \
    }))
#endif

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

int sethandler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        return -1;
    return 0;
}

int make_local_socket(char *name, struct sockaddr_un *addr)
{
    int socketfd;
    if ((socketfd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0)
        ERR("socket");
    memset(addr, 0, sizeof(struct sockaddr_un));
    addr->sun_family = AF_UNIX;
    strncpy(addr->sun_path, name, sizeof(addr->sun_path) - 1);
    return socketfd;
}

int connect_local_socket(char *name)
{
    struct sockaddr_un addr;
    int socketfd;
    socketfd = make_local_socket(name, &addr);
    if (connect(socketfd, (struct sockaddr *)&addr, SUN_LEN(&addr)) < 0)
    {
        ERR("connect");
    }
    return socketfd;
}

int bind_local_socket(char *name, int backlog_size)
{
    struct sockaddr_un addr;
    int socketfd;
    if (unlink(name) < 0 && errno != ENOENT)
        ERR("unlink");
    socketfd = make_local_socket(name, &addr);
    if (bind(socketfd, (struct sockaddr *)&addr, SUN_LEN(&addr)) < 0)
        ERR("bind");
    if (listen(socketfd, backlog_size) < 0)
        ERR("listen");
    return socketfd;
}

int make_tcp_socket(void)
{
    int sock;
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        ERR("socket");
    return sock;
}

struct sockaddr_in make_address(char *address, char *port)
{
    int ret;
    struct sockaddr_in addr;
    struct addrinfo *result;
    struct addrinfo hints = {};
    hints.ai_family = AF_INET;
    if ((ret = getaddrinfo(address, port, &hints, &result)))
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        exit(EXIT_FAILURE);
    }
    addr = *(struct sockaddr_in *)(result->ai_addr);
    freeaddrinfo(result);
    return addr;
}

int connect_tcp_socket(char *name, char *port)
{
    struct sockaddr_in addr;
    int socketfd;
    socketfd = make_tcp_socket();
    addr = make_address(name, port);
    if (connect(socketfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0)
    {
        ERR("connect");
    }
    return socketfd;
}

int bind_tcp_socket(uint16_t port, int backlog_size)
{
    struct sockaddr_in addr;
    int socketfd, t = 1;
    socketfd = make_tcp_socket();
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t)))
        ERR("setsockopt");
    if (bind(socketfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        ERR("bind");
    if (listen(socketfd, backlog_size) < 0)
        ERR("listen");
    return socketfd;
}

ssize_t bulk_read(int fd, char *buf, size_t count)
{
    int c;
    size_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if (c < 0)
            return c;
        if (0 == c)
            return len;
        buf += c;
        len += c;
        count -= c;
    } while (count > 0 && *(buf - 1) != '$');
    return len;
}

ssize_t bulk_write(int fd, char *buf, size_t count)
{
    int c;
    size_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if (c < 0)
            return c;
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

#define MAX_CLIENTS 8

int clients[MAX_CLIENTS];

int add_new_client(int sfd)
{
    int nfd;
    if ((nfd = TEMP_FAILURE_RETRY(accept(sfd, NULL, NULL))) < 0)
    {
        if (EAGAIN == errno || EWOULDBLOCK == errno)
            return -1;
        ERR("accept");
    }

    char address;
    if (bulk_read(nfd, &address, 1) != 1)
        ERR("read");

    address -= '0';
    printf("Client %d connected\n", address);

    if (address < 1 || address > MAX_CLIENTS || clients[address - 1] != 0)
    {
        char *msg = "Wrong address";
        bulk_write(nfd, msg, strlen(msg));
        close(nfd);
        return -1;
    }

    clients[address - 1] = nfd;
    char send_address = address + '0';
    bulk_write(nfd, &send_address, 1);
    return nfd;
}

#define MAX_EVENTS 10

int main(int argc, char **argv)
{
    clients[0] = 0;
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <ip> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *ip = argv[1];
    char *port = argv[2];

    int sfd = bind_tcp_socket(atoi(port), MAX_CLIENTS);

    // Create an epoll instance
    int efd = epoll_create1(0);
    if (efd == -1)
        ERR("epoll_create1");

    struct epoll_event event;
    event.data.fd = sfd;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &event) == -1)
        ERR("epoll_ctl");

    struct epoll_event events[MAX_EVENTS];

    while (1)
    {
        int n = epoll_wait(efd, events, MAX_EVENTS, -1);
        for (int i = 0; i < n; i++)
        {
            if (events[i].data.fd == sfd)
            {
                int cfd = add_new_client(sfd);
                event.data.fd = cfd;
                event.events = EPOLLIN | EPOLLET;
                if (epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &event) == -1)
                    ERR("epoll_ctl");
            }
            else
            {
                char buffer[1024];
                ssize_t n = read(events[i].data.fd, buffer, sizeof(buffer) - 1);
                if (n == -1)
                {
                    if (errno != EAGAIN)
                    {
                        perror("read");
                    }
                    continue;
                }
                else if (n == 0)
                {
                    continue;
                }

                buffer[n] = '\0';
                printf("Received data from client %d: %s\n", events[i].data.fd, buffer);
                write(events[i].data.fd, buffer, n);
            }
        }
    }

    return EXIT_SUCCESS;
}