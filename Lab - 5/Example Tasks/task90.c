#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L // Added bc macos doestnt have it yey

#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#ifndef TEMP_FAILURE_RETRY // Added bc macos doestnt have it yey
#define TEMP_FAILURE_RETRY(expr) \
    ({ long int _res; \
     do _res = (long int) (expr); \
     while (_res == -1L && errno == EINTR); \
     _res; })
#endif

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s fifo_file\n", name);
    exit(EXIT_FAILURE);
}

void parent_work(int N, int rounds, int send_pipes[][2], int recv_pipes[][2])
{
    for (int round = 0; round < rounds; round++)
    {
        printf("Starting round %d\n", round + 1);

        for (int i = 0; i < N; i++)
        {
            char start_msg[10] = "start";
            if (write(send_pipes[i][1], start_msg, sizeof(start_msg)) == -1)
                ERR("write");

            char card[10];
            if (read(recv_pipes[i][0], card, 10) <= 0)
                ERR("read");
            printf("Got number %s from player %d\n", card, i);
        }
    }
}

void child_work(int M, int i, int send_pipes[][2], int recv_pipes[][2])
{
    char start_msg[10];
    if (read(send_pipes[i][0], start_msg, 10) <= 0)
        ERR("read");

    if (strcmp(start_msg, "start") == 0)
    {
        // Child process (player)
        char buf[10];
        int card = rand() % M + 1;
        sprintf(buf, "%d", card);
        printf("Player %d is sending card %d\n", i, card);
        if (write(recv_pipes[i][1], buf, sizeof(buf)) == -1)
            ERR("write");
    }
}

int main(int argc, char **argv)
{
    srand(time(NULL));
    if (argc != 3)
        usage(argv[0]);
    int N, M;
    N = atoi(argv[1]);
    M = atoi(argv[2]);

    if (N < 2 || N > 5 || M < 5 || M > 10)
        usage(argv[0]);

    int send_pipes[N][2];
    int recv_pipes[N][2];

    for (int i = 0; i < N; i++)
    {
        if (pipe(send_pipes[i]) == -1)
            ERR("pipe");
        if (pipe(recv_pipes[i]) == -1)
            ERR("pipe");

        switch (fork())
        {
        case -1:
            ERR("fork");
        case 0:
            child_work(M, i, send_pipes, recv_pipes);
            break;
        default:
            // Parent process (server)
            break;
        }
    }
    parent_work(N,M, send_pipes, recv_pipes);

    return 0;
}