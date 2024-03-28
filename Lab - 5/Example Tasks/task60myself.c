#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L // Added bc macos doestnt have it yey

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
#include <stdlib.h>
#include <time.h>

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

int start = 0;

void child_work(int readp, int writep)
{
    printf("PID %d, I'm a child, I read from %d, write to %d\n", getpid(), readp, writep);

    // Read the message from the previous process
    while (1)
    {
        char buffer[256];
        ssize_t n = read(readp, buffer, sizeof(buffer) - 1);
        if (n == -1)
            ERR("read");
        buffer[n] = '\0'; // Null-terminate the string
        printf("PID %d read: %s\n", getpid(), buffer);

        // Generate a random number
        srand(time(NULL) ^ (getpid() << 16));
        int random_number = rand() %100;

        // Send a message to the next process
        char message[256];
        snprintf(message, sizeof(message), "%d", random_number);
        if (write(writep, message, strlen(message)) == -1)
            ERR("write");
        printf("PID %d wrote: %s\n", getpid(), message);
    }
    /*char buf[10];
    int n;
    ssize_t count;
    while ((count = (read(readp, buf, sizeof(buf) - 1)) > 0))
    {
        if (count <= 0)
            break;
        buf[count] = '\0'; // Null-terminate the string
        n = atoi(buf);
        if (n == 1 && flag==0)
        {
            printf("PID %d read: %s\n", getpid(), buf);
            sprintf(buf, "%d", 1);
            if (TEMP_FAILURE_RETRY(write(writep, buf, strlen(buf) + 1)) == -1)
                ERR("write");
            printf("PID %d wrote: %s\n", getpid(), buf);
            flag=1;
            continue;
        }
        else
        {

            printf("PID %d read: %s\n", getpid(), buf);
            n += rand() % 21 - 10;
            sprintf(buf, "%d", n);
            if (TEMP_FAILURE_RETRY(write(writep, buf, strlen(buf) + 1)) == -1)
                ERR("write");
            printf("PID %d wrote: %s\n", getpid(), buf);
        }
    }*/
    /*
        char c, buf[100 + 1];
        unsigned char s;
        srand(getpid());
        while (1)
        {



        for (;;)
        {
            ssize_t count;
            if ((count = TEMP_FAILURE_RETRY(read(readp, &c, 1))) < 1)
            {
                ERR("read");
                continue;
            }
        }
    s = (c - '0') + rand() % 21 - 10; // Convert the character to an integer
        buf[0] = s;
        memset(buf + 1, c, s);
        if (TEMP_FAILURE_RETRY(write(writep, buf, s + 1)) < 0)
            ERR("write to R");
        }*/
}

void parent_work(int readp, int writep)
{

    while (1)
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
        char buffer[256];
        ssize_t n = read(readp, buffer, sizeof(buffer) - 1);
        if (n == -1)
            ERR("read");
        buffer[n] = '\0'; // Null-terminate the string
        printf("PID %d read: %s\n", getpid(), buffer);
    }
    /*
        char buf[10];
        sprintf(buf, "%d", 1);
        if (TEMP_FAILURE_RETRY(write(writep, buf, strlen(buf))) == -1)
            ERR("write");

        while (1)
        {
            char buf[10];
            int n;
            while (TEMP_FAILURE_RETRY(read(readp, buf, sizeof(buf)) > 0))
            {

                n = atoi(buf);
                printf("PID %d read: %s\n", getpid(), buf);
                n += (rand() % 21) - 10;
                sprintf(buf, "%d", n);
                if (TEMP_FAILURE_RETRY(write(writep, buf, strlen(buf) + 1)) == -1)
                    ERR("write");
                printf("PID %d wrote: %s\n", getpid(), buf);
            }
        }*/
}

void create_pipes_and_children(int **fds, int n)
{
    int q = n;
    n--;
    while (n >= 0)
    {
        if (pipe(fds[n]))
            ERR("pipe");
        n--;
    }

    for (int i = 1; i < q; i++)
    {
        switch (fork())
        {
        case 0:
            for (int j = 0; j < q; j++)
            {
                if (j != i - 1) // Don't close the read end of the current pipe
                {
                    if (TEMP_FAILURE_RETRY(close(fds[j][0])))
                        ERR("close");
                }
                if (j != i) // Don't close the write end of the next pipe
                {
                    if (TEMP_FAILURE_RETRY(close(fds[j][1])))
                        ERR("close");
                }
            }

            child_work(fds[(i + q - 1) % q][0], fds[i][1]);
            exit(EXIT_SUCCESS);
        case -1:
            ERR("fork");
        }
    }
    for (int i = 0; i < q - 1; i++)
    {
        if (TEMP_FAILURE_RETRY(close(fds[i][0])))
            ERR("close");
    }
    for (int i = 1; i < q; i++)
    {
        if (TEMP_FAILURE_RETRY(close(fds[i][1])))
            ERR("close");
    }
    parent_work(fds[q - 1][0], fds[0][1]);
}

int main(int argc, char **argv)
{

    int n = 7;
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