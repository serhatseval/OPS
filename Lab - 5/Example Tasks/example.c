#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ROULETTE_NUMS 37

#define UNUSED(x) ((void)(x))

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

#ifndef TEMP_FAILURE_RETRY // Added bc macos doestnt have it yey
#define TEMP_FAILURE_RETRY(expr) \
    ({ long int _res; \
     do _res = (long int) (expr); \
     while (_res == -1L && errno == EINTR); \
     _res; })
#endif

volatile sig_atomic_t last_signal = 0;

int sethandler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        return -1;
    return 0;
}

void sig_handler(int sig) { last_signal = sig; }

void sigchld_handler(int sig)
{
    sig = sig;
    pid_t pid;
    for (;;)
    {
        pid = waitpid(0, NULL, WNOHANG);
        if (0 == pid)
            return;
        if (0 >= pid)
        {
            if (ECHILD == errno)
                return;
            ERR("waitpid:");
        }
    }
}

void child_work(int M, int fw, int fr)
{
    int am, num, lucky;
    srand(getpid());

    printf("[%d] I have %d$ and I'm going to play roullete\n", getpid(), M);

    for (;;)
    {
        if (rand() % 10 == 0)
        {
            printf("[%d] I saved %d$\n", getpid(), M);
            break;
        }

        am = rand() % M + 1;
        num = rand() % ROULETTE_NUMS;

        if (TEMP_FAILURE_RETRY(write(fw, &am, sizeof(int))) < 0)
            ERR("write");
        if (TEMP_FAILURE_RETRY(write(fw, &num, sizeof(int))) < 0)
            ERR("write");

        if (TEMP_FAILURE_RETRY(read(fr, &lucky, sizeof(int))) < (long int)sizeof(int))
            ERR("read");

        if (lucky == num)
        {
            printf("[%d] Whoa, I won %d$\n", getpid(), 35 * am);
            M += 35 * am;
        }
        else
        {
            M -= am;
        }

        if (M <= 0)
        {
            printf("[%d] I'm broke\n", getpid());
            break;
        }
    }
}

void parent_work(int N, int *fw, int *fr, int *pids)
{
    int lucky, players = N;
    srand(getpid());

    while (1)
    {
        for (int i = 0; i < N; i++)
        {
            int am, num, status;

            if (fw[i] == 0)
                continue;

            if ((status = read(fr[i], &am, sizeof(int))) < 1)
            {
                if (status < 0 && errno == EINTR)
                {
                    i--;
                    continue;
                }
                if (status == 0)
                {
                    if (TEMP_FAILURE_RETRY(close(fw[i])))
                        ERR("close");
                    if (TEMP_FAILURE_RETRY(close(fr[i])))
                        ERR("close");

                    fw[i] = 0;
                    fr[i] = 0;

                    players--;

                    continue;
                }
            }
            if (TEMP_FAILURE_RETRY(read(fr[i], &num, sizeof(int))) < 1)
                ERR("read");

            printf("Croupier: [%d] placed %d$ on a %d\n", pids[i], am, num);
        }

        if (players <= 0)
            break;

        lucky = rand() % ROULETTE_NUMS;

        printf("\nCroupier: %d is the lucky number\n\n", lucky);

        for (int i = 0; i < N; i++)
        {
            if (fw[i] == 0)
                continue;

            if ((TEMP_FAILURE_RETRY(write(fw[i], &lucky, sizeof(int)))) < 0)
                ERR("write");
        }
    }

    printf("\nCroupier: Casino always wins\n");
}

void create_children_and_pipes(int N, int M, int *fw, int *fr, int *pids)
{
    int max = N, tfw[2], tfr[2], npid;
    while (N)
    {
        if (pipe(tfw))
            ERR("pipe");
        if (pipe(tfr))
            ERR("pipe");
        switch (npid = fork())
        {
            case 0:
                while (N < max)
                {
                    if (TEMP_FAILURE_RETRY(close(fw[N++])))
                        ERR("close");
                    if (TEMP_FAILURE_RETRY(close(fr[N++])))
                        ERR("close");
                }
                free(fw);
                free(fr);

                if (TEMP_FAILURE_RETRY(close(tfw[1])))
                    ERR("close");
                if (TEMP_FAILURE_RETRY(close(tfr[0])))
                    ERR("close");

                child_work(M, tfr[1], tfw[0]);

                if (TEMP_FAILURE_RETRY(close(tfw[0])))
                    ERR("close");
                if (TEMP_FAILURE_RETRY(close(tfr[1])))
                    ERR("close");

                exit(EXIT_SUCCESS);
            case -1:
                ERR("Fork:");
        }

        if (TEMP_FAILURE_RETRY(close(tfw[0])))
            ERR("close");
        if (TEMP_FAILURE_RETRY(close(tfr[1])))
            ERR("close");

        N--;

        fw[N] = tfw[1];
        fr[N] = tfr[0];
        pids[N] = npid;
    }
}

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s N M\n", name);
    fprintf(stderr, "N: N >= 1 - number of players\n");
    fprintf(stderr, "M: M >= 100 - initial amount of money\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    int N, M, *fw, *fr, *pids;
    if (3 != argc)
        usage(argv[0]);
    N = atoi(argv[1]);
    if (N < 1)
        usage(argv[0]);
    M = atoi(argv[2]);
    if (M < 100)
        usage(argv[0]);

    if (sethandler(SIG_IGN, SIGCHLD))
        ERR("Setting parent SIGCHLD:");
    if (NULL == (fw = (int *)malloc(sizeof(int) * N)))
        ERR("malloc");
    if (NULL == (fr = (int *)malloc(sizeof(int) * N)))
        ERR("malloc");
    if (NULL == (pids = (int *)malloc(sizeof(int) * N)))
        ERR("malloc");

    create_children_and_pipes(N, M, fw, fr, pids);

    parent_work(N, fw, fr, pids);

    for (int i = 0; i < N; i++)
    {
        wait(NULL);
    }

    free(fw);
    free(fr);
    free(pids);

    return EXIT_SUCCESS;
}