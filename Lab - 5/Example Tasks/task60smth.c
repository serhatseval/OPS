#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
void usage(char *name)
{
    fprintf(stderr, "USAGE: %s fifo_file\n", name);
    exit(EXIT_FAILURE);
}
void child_work(int readp, int writep)
{
    printf("PID %d, I'm a child, I read from %d, write to %d\n", getpid(), readp, writep);

    // Read the message from the previous process
    int random_number = 0;
    while (1)
    {
        char buffer[256];
        ssize_t n = read(readp, buffer, sizeof(buffer) - 1);
        if (n == -1)
        {
            if (errno == EPIPE) // Broken pipe means the other process has terminated
            {
                close(readp);
                close(writep);
                exit(EXIT_SUCCESS);
            }
            ERR("read");
        }
        buffer[n] = '\0'; // Null-terminate the string
        random_number = atoi(buffer);
        printf("PID %d read: %s\n", getpid(), buffer);

        // Terminate if the received number is 0
        if (random_number == 0) {
            close(readp);
            close(writep);
            exit(EXIT_SUCCESS);
        }

        // Generate a random number
        srand(time(NULL) ^ (getpid() << 16));
        random_number += rand() % 21 - 10;

        // Send a message to the next process
        char message[256];
        snprintf(message, sizeof(message), "%d", random_number);
        if (write(writep, message, strlen(message)) == -1)
        {
            if (errno == EPIPE) // Broken pipe means the other process has terminated
            {
                close(readp);
                close(writep);
                exit(EXIT_SUCCESS);
            }
            ERR("write");
        }
        printf("PID %d wrote: %s\n", getpid(), message);
    }
}

void parent_work(int readp, int writep)
{
    printf("PID %d, I'm a parent, I read from %d, write to %d\n", getpid(), readp, writep);

    // Generate a random number
    srand(time(NULL) ^ (getpid() << 16));
    int random_number = rand() % 100;

    // Send a message to the first child process
    char message[256];
    snprintf(message, sizeof(message), "%d", random_number);
    if (write(writep, message, strlen(message)) == -1)
        ERR("write");

    printf("PID %d wrote: %s\n", getpid(), message);

    // Read the message from the last child process
    while (1)
    {
        char buffer[256];
        ssize_t n = read(readp, buffer, sizeof(buffer) - 1);
        if (n == -1)
        {
            if (errno == EPIPE) // Broken pipe means the other process has terminated
            {
                close(readp);
                close(writep);
                exit(EXIT_SUCCESS);
            }
            ERR("read");
        }
        buffer[n] = '\0'; // Null-terminate the string
        printf("PID %d read: %s\n", getpid(), buffer);

        // Generate a random number
        random_number = atoi(buffer) + rand() % 21 - 10;

        // Send a message to the first child process
        snprintf(message, sizeof(message), "%d", random_number);
        if (write(writep, message, strlen(message)) == -1)
        {
            if (errno == EPIPE) // Broken pipe means the other process has terminated
            {
                close(readp);
                close(writep);
                exit(EXIT_SUCCESS);
            }
            ERR("write");
        }
        printf("PID %d wrote: %s\n", getpid(), message);
    }
}

void create_pipes_and_children(int **fds, int n){
    for (int i = 0; i < n; i++)
    {
        if (pipe(fds[i]))
            ERR("pipe");
    }

    for (int i = 0; i < n; i++)
    {
        switch (fork())
        {
        case 0:
            for (int j = 0; j < n; j++)
            {
                if (j != i) // Don't close the read end of the current pipe
                    close(fds[j][0]);
                if ((j + 1) % n != i) // Don't close the write end of the next pipe
                    close(fds[j][1]);
            }
            child_work(fds[i][0], fds[(i + 1) % n][1]);
            exit(EXIT_SUCCESS);
        case -1:
            ERR("fork");
        }
    }

    for (int i = 0; i < n; i++)
    {
        close(fds[i][0]);
        close(fds[i][1]);
    }

    parent_work(fds[0][0], fds[n - 1][1]);
}

int main(int argc, char **argv){
    int n = 5;
    int **fds = malloc(n * sizeof(int *));
    if (fds == NULL)
    {
        ERR("malloc");
    }
    for (int i = 0; i < n; i++)
    {
        fds[i] = malloc(2 * sizeof(int));
        if (fds[i] == NULL)
        {
            ERR("malloc");
        }
    }
    create_pipes_and_children(fds, n);
    for (int i = 0; i < n; i++)
    {
        wait(NULL); // Wait for all child processes to terminate
    }
    for (int i = 0; i < n; i++)
    {
        free(fds[i]);
    }
    free(fds);
    return 0;
}