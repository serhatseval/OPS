#include "common.h"

#define MAX_EVENTS 10
volatile sig_atomic_t max_sum = 0;
volatile sig_atomic_t interrupted = 0;

void handle_sigint(int sig)
{
    interrupted = 1;
    printf("Max sum: %d\n", max_sum);
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
    if (argc < 2)
        ERR("Usage:  <port>");

    int port = atoi(argv[1]);

    int server_fd, client_fd;
    char buffer[1024];

    server_fd = bind_tcp_socket(port, 1);

    struct epoll_event ev, events[MAX_EVENTS];
    int nfds, epollfd;
    sethandler(handle_sigint, SIGINT);

    // Create an epoll instance
    epollfd = epoll_create1(0);
    if (epollfd == -1)
    {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    // Add the server file descriptor to the epoll instance
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, server_fd, &ev) == -1)
    {
        perror("epoll_ctl: server_fd");
        exit(EXIT_FAILURE);
    }

    for(;;)
    {
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1)
        {
            if (errno == EINTR && interrupted)
            {
                printf("Max sum: %d\n", max_sum);
                break;
            }
            else
            {
                ERR("epoll_wait");
            }
        }

        for (int n = 0; n < nfds; ++n)
        {
            if (events[n].data.fd == server_fd)
            {
                // Accept new client connection
                client_fd = add_new_client(server_fd);
                printf("New client connected\n");
                if (client_fd < 0)
                    ERR("add_new_client");

                // Add the client file descriptor to the epoll instance
                ev.events = EPOLLIN;
                ev.data.fd = client_fd;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, client_fd, &ev) == -1)
                {
                    perror("epoll_ctl: client_fd");
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                // Handle client connection
                int16_t sum = 0;
                if (bulk_read(events[n].data.fd, buffer, sizeof(buffer) - 1) < 0)
                    ERR("bulk_read");

                for (int i = 0; i < strlen(buffer); i++)
                {
                    sum += buffer[i] - '0';
                }

                if (sum > max_sum)
                    max_sum = sum;

                printf("Sum: %d\n", sum);

                // Close the client connection and remove it from the epoll instance
                if (epoll_ctl(epollfd, EPOLL_CTL_DEL, events[n].data.fd, NULL) == -1)
                {
                    perror("epoll_ctl: client_fd");
                    exit(EXIT_FAILURE);
                }
                close(events[n].data.fd);
            }
        }
    }
}