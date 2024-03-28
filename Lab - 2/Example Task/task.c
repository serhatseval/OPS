#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

volatile sig_atomic_t last_signal = 0;

void sethandler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void sig_handler(int sig)
{
    printf("[%d] received signal %d\n", getpid(), sig);
    last_signal = sig;
}

void sigchld_handler(int sig)
{
    pid_t pid;
    for (;;)
    {
        pid = waitpid(0, NULL, WNOHANG);
        if (pid == 0)
            return;
        if (pid <= 0)
        {
            if (errno == ECHILD)
                return;
            ERR("waitpid");
        }
    }
}

void child_work(){
    srand(time(NULL) * getpid());
    int t = 100 + rand() % (101);
    write("PROCESS with pid %d chose %d\n", getpid(),t);
    fflush(stdout);
}


void create_children(int n)
{
    while (n-- > 0)
    {
        switch (fork())
        {
            case 0:
                sethandler(sig_handler, SIGUSR1);
                sethandler(sig_handler, SIGUSR2);
                child_work();
                exit(EXIT_SUCCESS);
            case -1:
                perror("Fork:");
                exit(EXIT_FAILURE);
        }
    }
}



int main(int argc, char** argv){

    if (argc != 2)
    {
        printf("Wrong number of arguments\n");
        exit(EXIT_FAILURE);
    }

    int n = atoi(argv[1]);

    if (n <= 0)
    {
        printf("Wrong number of children\n");
        exit(EXIT_FAILURE);
    }

    create_children(n);
    


    return EXIT_SUCCESS;
}