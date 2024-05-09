#include "l4-common.h"

#define BACKLOG 3
#define MAX_EVENTS 16
#define MAX_CLIENTS 4
#define MAX_CITIES 20

volatile sig_atomic_t do_work = 1;

void sigint_handler(int sig)
{
    do_work = 0;
    sig = sig;
}

int atPos(int *tab, int am, int el)
{
    for (int i = 0; i < am; i++)
    {
        if (tab[i] == el)
            return i;
    }
    return -1;
}

void doServer(int local_listen_socket, int tcp_listen_socket)
{
    int epoll_descriptor;
    if ((epoll_descriptor = epoll_create1(0)) < 0)
    {
        ERR("epoll_create:");
    }
    struct epoll_event event, events[MAX_EVENTS];
    event.events = EPOLLIN;
    event.data.fd = local_listen_socket;
    if (epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, local_listen_socket, &event) == -1)
    {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }

    event.data.fd = tcp_listen_socket;
    if (epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, tcp_listen_socket, &event) == -1)
    {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }

    int cities[MAX_CITIES];
    int clients[MAX_CLIENTS];
    int clients_amount = 0;
    int nfds;
    int8_t data[4];
    ssize_t size;
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);
    while (do_work)
    {
        if ((nfds = epoll_pwait(epoll_descriptor, events, MAX_EVENTS, -1, &oldmask)) > 0)
        {
            for (int n = 0; n < nfds; n++)
            {
                int client_socket;
                int recieved = 0;
                if (events[n].data.fd == tcp_listen_socket)
                {
                    client_socket = add_new_client(events[n].data.fd);

                    if (clients_amount == MAX_CLIENTS)
                    {
                        close(client_socket);
                        printf("Ignored new client\n");
                        continue;
                    }

                    if ((size = bulk_read(client_socket, (char *)data, sizeof(int8_t[4]))) < 0)
                        ERR("read:");

                    recieved = 1;
                    printf("%d, %d, %d, %d\n", data[0], data[1], data[2], data[3]);

                    if (bulk_write(client_socket, (char *)data, sizeof(int8_t[4])) < 0 && errno != EPIPE)
                        ERR("write:");

                    event.data.fd = client_socket;
                    if (epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, client_socket, &event) == -1)
                    {
                        perror("epoll_ctl: listen_sock");
                        exit(EXIT_FAILURE);
                    }

                    clients[clients_amount++] = client_socket;
                }
                else
                {
                    int pos = atPos(clients, clients_amount, events[n].data.fd);

                    if (pos < 0)
                        ERR("Unknown");

                    client_socket = clients[pos];

                    if ((size = bulk_read(client_socket, (char *)data, sizeof(int8_t[4]))) < 0)
                        ERR("read:");
                    if (size == (int)sizeof(int8_t[4]))
                    {
                        recieved = 1;
                        printf("%d, %d, %d, %d\n", data[0], data[1], data[2], data[3]);
                        if (bulk_write(client_socket, (char *)data, sizeof(int8_t[4])) < 0 && errno != EPIPE)
                            ERR("write:");
                    }
                }

                if (recieved)
                {
                    int city = (data[1] - '0') * 10 + data[2] - '0' - 1;

                    if (city < 0 || city > MAX_CITIES - 2)
                        continue;

                    int owner = -1;

                    if (data[0] == 'p')
                        owner = 1;

                    if (data[0] == 'g')
                        owner = 0;

                    if (owner == -1 || cities[city] == owner)
                        continue;
                    ;

                    cities[city] = owner;

                    for (int i = 0; i < MAX_CLIENTS; i++)
                    {
                        if (clients[i] == 0 || clients[i] == client_socket)
                            continue;

                        if (bulk_write(clients[i], (char *)data, sizeof(int8_t[4])) < 0 && errno != EPIPE)
                            ERR("write:");
                    }
                }
            }
        }
        else
        {
            if (errno == EINTR)
                continue;
            ERR("epoll_pwait");
        }
    }
    if ((close(epoll_descriptor)) < 0)
        ERR("close");
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

int main(int argc, char **argv)
{
    int local_listen_socket, tcp_listen_socket;
    int new_flags;
    if (argc != 2)
    {
        usage(argv[0]);
        return EXIT_FAILURE;
    }
    if (sethandler(SIG_IGN, SIGPIPE))
        ERR("Seting SIGPIPE:");
    // if (sethandler(sigint_handler, SIGINT))
    // ERR("Seting SIGINT:");
    local_listen_socket = bind_local_socket(argv[1], BACKLOG);
    new_flags = fcntl(local_listen_socket, F_GETFL) | O_NONBLOCK;
    fcntl(local_listen_socket, F_SETFL, new_flags);
    tcp_listen_socket = bind_tcp_socket(atoi(argv[1]), BACKLOG);
    new_flags = fcntl(tcp_listen_socket, F_GETFL) | O_NONBLOCK;
    fcntl(tcp_listen_socket, F_SETFL, new_flags);
    doServer(local_listen_socket, tcp_listen_socket);
    if ((close(local_listen_socket)) < 0)
        ERR("close");
    if (unlink(argv[1]) < 0)
        ERR("unlink");
    if ((close(tcp_listen_socket)) < 0)
        ERR("close");
    fprintf(stderr, "Server has terminated.\n");
    return EXIT_SUCCESS;
}
