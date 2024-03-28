#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>


void sethandler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL)){}
        //ERR("sigaction");
}

void sighandler(int sig){
    printf("Signal %d received\n", sig);
}

int main(int argc, char** argv){

    printf("PID: %d\n", getpid());
    return EXIT_SUCCESS;
}