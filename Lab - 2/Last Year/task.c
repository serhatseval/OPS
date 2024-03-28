#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#define ERR(source)

volatile sig_atomic_t last_signal = 0;

int sig_count = 0; // i from ex
int dwa = 0;

typedef struct data
{
    pid_t pid;
    int k;
    int i;
} Data;

void sethandler (void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void sig_handler()
{
    sig_count++;
    pid_t pid = getpid();
    printf("%d count: %d\n", pid, sig_count);
    fflush(stdout);
}

int child_work()
{

    pid_t pid = getpid();
    srand(getpid());
    int k = rand() % 11;
    printf("pid: %d k: %d\n", pid, k);

    for(int i = 0; i < k; i++)
    {
        kill(0, SIGUSR1);
        sleep(1);
        sleep(1);
        //sleep(1);
    }
    return k;
}

void create_children(int num)
{
    while (num-- >0)
    {
        switch(fork())
        {
            //bachor
            case 0:
                sethandler(sig_handler, SIGUSR1);
                int new_k = child_work();
                //printf("probuje :%d\n", new_k);
                //printf("Witek placze: %d\n", sig_count);
                int ex = sig_count*10 + new_k;
                //printf("ex :%d\n", ex);
                exit(ex);
            default:
                continue;
        }
    }

}

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s n*\n", name);
    fprintf(stderr, "n - digits for block\n");    
    exit(EXIT_FAILURE);
}

int compare_data(const void *a, const void *b) {
    return ((Data *)a)->k - ((Data *)b)->k;
}

int main (int argc, char **argv)
{
    if(argc != 2)
        usage(argv[0]);
    int n = atoi(argv[1]);
    sethandler(SIG_IGN,SIGUSR1);
    create_children(n);
    pid_t pid;
    int status;
    Data temp[n];
    int size=n;
    for (;;)
    {
        pid = waitpid(0, &status, WUNTRACED);
        if (pid > 0)
        {
            n--;
            int es = WEXITSTATUS(status);
            //printf("Exit status was %d\n", es);
            printf("(%d, %d, %d)\n", pid, es%10, es/10);
            temp[n].pid = getpid();  
            temp[n].k = es % 10;
            temp[n].i = es / 10;
    
            //printf("n: %d\n", n);
        }
        if (0 == pid)
            break;    
        if (0 >= pid)
        {
            if (ECHILD == errno)
                break;
                ERR("waitpid:");
        }
    }
    
    qsort(temp, size, sizeof(Data), compare_data);

    freopen("out.txt", "a+", stdout); 
    printf("\nSorted array by 'k':\n");
    for (int i = 0; i < size; ++i) {
        printf("pid: %d, k: %d, i: %d\n", temp[i].pid, temp[i].k, temp[i].i);
    }
    exit(EXIT_SUCCESS);

}