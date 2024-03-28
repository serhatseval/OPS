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
    fprintf(stderr, "USAGE: %s 2<N<5 5<M<10\n", name);
    exit(EXIT_FAILURE);
}

void child_work(int R, int W)
{
}

void create_children_and_pipes(int n, int **fds) // Use this to create n children pipes which is connected to parent
{
    int tmpfd[2][2];
    int max = n;
    while (n)
    {
        if (pipe(tmpfd[0]) || pipe(tmpfd[1]))
            ERR("pipe");
        switch (fork())
        {
            switch (fork())
            {
            case 0:
                while (n < max)
                {
                    // Close the unused ends of the pipes in the child
                    if (fds[n] && TEMP_FAILURE_RETRY(close(fds[n][0])) && TEMP_FAILURE_RETRY(close(fds[n][1])))
                        ERR("close");
                    n++;
                }
                free(fds);
                // Close the write end of the first pipe and the read end of the second pipe in the child
                if (TEMP_FAILURE_RETRY(close(tmpfd[0][1])) || TEMP_FAILURE_RETRY(close(tmpfd[1][0])))
                    ERR("close");
                child_work(tmpfd[0][0], tmpfd[1][1]);
                // Close the read end of the first pipe and the write end of the second pipe in the child
                if (TEMP_FAILURE_RETRY(close(tmpfd[0][0])) || TEMP_FAILURE_RETRY(close(tmpfd[1][1])))
                    ERR("close");
                exit(EXIT_SUCCESS);

            case -1:
                ERR("Fork:");
            }
            // Close the read end of the first pipe and the write end of the second pipe in the parent
            if (TEMP_FAILURE_RETRY(close(tmpfd[0][0])) || TEMP_FAILURE_RETRY(close(tmpfd[1][1])))
                ERR("close");
            fds[--n][0] = tmpfd[0][1];
            fds[n][1] = tmpfd[1][0];
        }
    }
}

int main(int argc, char **argv)
{
    int **fds;
    if (argc != 3)
    {
        usage(argv[0]);
    }
    int N = atoi(argv[1]);
    int M = atoi(argv[2]);
    if (N < 2 || N > 5 || M < 5 || M > 10)
    {
        usage(argv[0]);
    }

    fds = malloc(N * sizeof(int *));
    if (fds == NULL)
    {
        ERR("malloc");
    }
    for (int i = 0; i < N; i++)
    {
        fds[i] = malloc(2 * sizeof(int));
        if (fds[i] == NULL)
        {
            ERR("malloc");
        }
    }

    create_children_and_pipes(N, fds);

    // Don't forget to free the memory when you're done
    for (int i = 0; i < N; i++)
    {
        free(fds[i]);
    }
    free(fds);

    return(EXIT_SUCCESS);
}