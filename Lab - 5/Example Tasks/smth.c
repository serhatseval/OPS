#define _GNU_SOURCE 

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define MAX_BUFSIZE 1024

void child_work(int fds[3][2], int n)
{
    
    printf("PID: %d, FD(read): %d, FD(write): %d\n", getpid(), fds[n-1][0], fds[n][1]); 
    int x;
    if(read(fds[n-1][0], &x, sizeof(int)) == -1)
        ERR("read");

    //closing the reading end of a child(i)->child/parent
    if(close(fds[n-1][0]) == -1)
        ERR("close");
    
    
    printf("child %d received number: %d\n", getpid(), x);
    exit(EXIT_SUCCESS); 
}

void create_children_and_pipes(int n, int fds[3][2])
{
    
    pid_t s;
    for (int i = 1; i <= 2; i++)
    {
        if (pipe(fds[i]) == -1)
            ERR("pipe");
        //Close these pipes for parent
       
        s = fork();
        if (s == 0)
        {
            //closing the reading end of a child1->child2 / child2->parent
            if(close(fds[i][0]) == -1)
            ERR("close");

            srand(getpid());
            int x = rand()%99;
            printf("child %d of pid %d generated number: %d\n", i, getpid(), x);
            write(fds[i][1], &x, sizeof(x));

            //closing the writing end of a child(i)->child/parent
            if(close(fds[i][1]) == -1)
                ERR("close");

            child_work(fds, i);
        }
        else if (s < 0) //error 
        {
            //closing the reading end of a parent->child1
            close(fds[0][0]); //code will not go here
            ERR("fork");
        }
        else
        {
            
        }
    }
}

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s n\n", name);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    srand(time(NULL));

    if (argc != 1)
        usage(argv[0]);

    int fds[3][2];
    if (pipe(fds[0]) == -1)
            ERR("pipe");   
    
    //read is not closed
    
    int num1=rand()%99;
    printf("parent generates number: %d\n", num1);
    write(fds[0][1], &num1, sizeof(int));    

    //closing the writing end of a parent->child1
    close(fds[0][1]);

    create_children_and_pipes(2, fds);
    
    if(read(fds[2][0], &num1, sizeof(int)) == -1)
        ERR("read");

    //closing the reading end of a child2->parent
    if(close(fds[2][0]) == -1)
        ERR("close");

    printf("Parent PID: %d, FD(read): %d, FD(write): %d\n", getpid(), fds[2][0], fds[0][1]);
    printf("Parent received number: %d\n\n", num1);

    
    // Wait for all children to exit
    while (wait(NULL) > 0)
        ;

    return EXIT_SUCCESS;
}