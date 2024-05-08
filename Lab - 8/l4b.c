#include "common.h"
#include <time.h>

#define MAX_CLIENTS 10

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s <ip> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    char *log_f = argv[1];
    FILE *log = fopen(log_f, "a");
    if (log == NULL)
        ERR("fopen");

    char *port = argv[3];

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

    struct epoll_event events[10];

    while (1)
    {
        int n = epoll_wait(efd, events, 10, -1);
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

                // ...

                buffer[n] = '\0';
                printf("Received data from client %d: %s\n", events[i].data.fd, buffer + 1); // Skip first byte

                time_t now = time(NULL);
                char *timestamp = ctime(&now);
                timestamp[strlen(timestamp) - 1] = '\0'; // Remove newline character added by ctime

                printf("Buffer: %s\n", buffer + 1);
                fprintf(log, "[%s] Received data from client %d: %s\n", timestamp, events[i].data.fd, buffer + 1);
                fflush(log);

                write(events[i].data.fd, buffer + 1, n - 1); // Skip first byte

              }
        }
    }

    return EXIT_SUCCESS;
}