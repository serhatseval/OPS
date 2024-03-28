#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L //Added bc macos doestnt have it yey

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

#ifndef TEMP_FAILURE_RETRY //Added bc macos doestnt have it yey
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

int rand_range(int min, int max) {
    return rand() % (max - min + 1) + min;
}

void child_work(int readpp, int writepp){
    char buf[10];
    int n;
    ssize_t count;

    close(readpp+1);
    close(writepp-1);
    while ((count = TEMP_FAILURE_RETRY(read(readpp, buf, sizeof(buf) - 1))) > 0) {
        buf[count] = '\0';
        //printf("PID: %d, received: %s\n", getpid(), buf);

        if (strcmp(buf, "STOP") == 0) {
            break;
        }

        n = atoi(buf);
        printf("PID: %d, received: %d\n", getpid(), n);

        n = rand_range(0, 99);
        sprintf(buf, "%d", n);

        if (write(writepp, buf, strlen(buf)) == -1) {
            break;
        }
    }

    close(readpp);
    close(writepp);
}

int main(void) {
    srand(time(NULL));

    int pipe1[2], pipe2[2], pipe3[2];

    if (pipe(pipe1) == -1 || pipe(pipe2) == -1 || pipe(pipe3) == -1) {
        ERR("pipe");
    }

    switch (fork()) {
        case 0:
            child_work(pipe1[0], pipe2[1]);
            exit(EXIT_SUCCESS);
        case -1:
            ERR("fork");
    }

    switch (fork()) {
        case 0:
            child_work(pipe2[0], pipe3[1]);
            exit(EXIT_SUCCESS);
        case -1:
            ERR("fork");
    }

    switch (fork()) {
        case 0:
            child_work(pipe3[0], pipe1[1]);
            exit(EXIT_SUCCESS);
        case -1:
            ERR("fork");
    }

    close(pipe1[0]);
    close(pipe2[0]);
    close(pipe3[0]);
    close(pipe2[1]);
    close(pipe3[1]);

    if (write(pipe1[1], "1", 1) == -1) {
        ERR("write");
    }

    close(pipe1[1]);

    while (wait(NULL) > 0);

    return 0;
}