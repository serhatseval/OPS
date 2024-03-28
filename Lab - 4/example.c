#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

typedef struct threads{
    pthread_t pid;

} thread_t;

ssize_t bulk_read(int fd, char *buf, size_t count)
{
    int c;
    size_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if (c < 0)
            return c;
        if (c == 0)
            return len;
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

ssize_t bulk_write(int fd, char *buf, size_t count)
{
    int c;
    size_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if (c < 0)
            return c;
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

void* thread_work(void* args){
    thread_t* arg = args;
    printf("*\n");
}


int main(int argc, char** argv){
    if(argc!=4){
        printf("Wrong amount of input!");
        return EXIT_FAILURE;
    }
    int n,m;
    char* path;
    n = atoi(argv[1]);
    m = atoi(argv[2]);
    path = argv[3];
    thread_t* threads =(thread_t *) malloc(sizeof(thread_t)*5);

    int in;
    in = TEMP_FAILURE_RETRY(open(path, O_RDONLY));

    for(int i=0; i<5;i++){
        pthread_create(&threads[i].pid, NULL, thread_work, threads);
    }

    for(int i=0; i<5; i++){
        pthread_join(&threads[i].pid, NULL);
    }
    return EXIT_SUCCESS;
}