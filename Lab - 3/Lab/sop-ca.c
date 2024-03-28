#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) \
    (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define UNUSED(x) (void)(x)

typedef struct mystruct
{
    int *array;
    pthread_mutex_t *mutexes;
    int n;
    int p;
    unsigned int seed;
} mystruct;

int SigUsr1Received = 0;
int SigUsr2Received = 0;

void sethandler(void (*f)(int), int sigNo)
{  // Signal handler from website
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void sigusr1_handler(int sig)
{
    if (sig == SIGUSR1)
        SigUsr1Received = 1;
}

void sigusr2_handler(int sig)
{
    if (sig == SIGUSR2)
        SigUsr2Received = 1;
}

void usage(const char *pname)
{
    fprintf(stderr, "USAGE: %s n p\n", pname);
    exit(EXIT_FAILURE);
}

void getarguments(int argc, char *argv[], int *n, int *p)
{
    if (argc != 3)
    {
        usage(argv[0]);
    }
    *n = atoi(argv[1]);
    if (*n < 8 || *n > 256)
    {
        usage(argv[0]);
    }
    *p = atoi(argv[2]);
    if (*p < 1 || *p > 16)
    {
        usage(argv[0]);
    }
}

void prepareArray(mystruct *struct1)
{
    for (int i = 0; i < struct1->n; i++)
    {
        struct1->array[i] = i;
        pthread_mutex_init(&struct1->mutexes[i], NULL);
    }
}

void *printing(void *arg)
{
    mystruct *struct1 = arg;
    for (int i = 0; i < struct1->n; i++)
    {
        pthread_mutex_lock(&struct1->mutexes[i]);
    }
    for (int i = 0; i < struct1->n; i++)
    {
        printf("%d ", struct1->array[i]);
    }

    for (int i = struct1->n - 1; 0 <= i; i--)
    {
        pthread_mutex_unlock(&struct1->mutexes[i]);
    }
    fflush(stdout);
    return NULL;
}

void mainWork(sigset_t *oldMask, mystruct *struct1)
{
    while (sigsuspend(oldMask))
    {
        if (SigUsr1Received)
        {
            printf("Received SIGUSR1\n");
            int a = rand() % (struct1->n - 1);
            int b = rand() % (struct1->n - a);
            b = b + a;
            printf("%d %d\n", a, b);
            while (1)
            {
                pthread_mutex_lock(&struct1->mutexes[a]);
                pthread_mutex_lock(&struct1->mutexes[b]);
                int temp = struct1->array[a];
                struct1->array[a] = struct1->array[b];
                struct1->array[b] = temp;
                pthread_mutex_unlock(&struct1->mutexes[b]);
                pthread_mutex_unlock(&struct1->mutexes[a]);
                a = a + 1;
                b = b - 1;
                if (a >= b)
                    break;
                struct timespec sleep = {0, 5 * 1000000};
                nanosleep(&sleep, NULL);
            }

            printf("debugging2 %d, %d\n", struct1->array[a], struct1->array[b]);
            SigUsr1Received = 0;
        }
        else if (SigUsr2Received)
        {
            printf("Received SIGUSR2\n");
            pthread_t tid;
            pthread_attr_t attr;  // from the website
            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
            pthread_create(&tid, &attr, printing, struct1);
            pthread_attr_destroy(&attr);
            SigUsr2Received = 0;
        }
    }
}

void destroymutexes(pthread_mutex_t *mutexes, int i)
{
    for (int p = 0; p < i; p++)
    {
        pthread_mutex_destroy(&mutexes[p]);
    }
}

int main(int argc, char *argv[])
{
    int n, p;
    srand(time(NULL));
    getarguments(argc, argv, &n, &p);
    int *array = (int *)malloc(sizeof(mystruct) * n);
    pthread_mutex_t *mutexes = (pthread_mutex_t *)malloc(n * sizeof(pthread_mutex_t));
    mystruct struct1;
    struct1.array = array;
    struct1.mutexes = mutexes;
    struct1.n = n;
    struct1.p = p;
    struct1.seed = rand();
    prepareArray(&struct1);

    sethandler(sigusr1_handler, SIGUSR1);
    sethandler(sigusr2_handler, SIGUSR2);

    sigset_t oldMask, newMask;  // Signal codes from website
    sigemptyset(&newMask);
    sigaddset(&newMask, SIGUSR1);
    sigaddset(&newMask, SIGUSR2);

    if (pthread_sigmask(SIG_BLOCK, &newMask, &oldMask) != 0)
        ERR("SIG_BLOCK error");
    mainWork(&oldMask, &struct1);
    destroymutexes(mutexes, n);
    free(array);
    free(mutexes);
    exit(EXIT_SUCCESS);
}
