#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

void usage(char* name)
{
    fprintf(stderr, "USAGE: %s server_pid\n", name);
    exit(EXIT_FAILURE);
}

volatile sig_atomic_t last_one = 1;

void childwork(int N, int M, int* shared_memory, int position)
{
    //wait till parent sends the "new round" message
    for(int i=0; i<M; i++){
        while (shared_memory[0] != position) {
        sleep(1);
    }
    srand(time(NULL) ^ (getpid()<<16));
    int number = rand() % M+1;
    printf("Child %d: %d\n",getpid(), number);
    
    shared_memory[position] = number;
    
    shared_memory[0] = position + 1; // Notify the next child or the parent that it's their turn
    if(position == N) shared_memory[0] = 999; // Notify the parent that it's their turn
    }
    exit(EXIT_SUCCESS);
}

void parent_work(int N, int M, int* shared_memory, int* children_pids)
{
    int recieved = 0;
    for (int round = 1; round <= M; round++) {
        shared_memory[0] = 1;  // Start the first child
        printf("Round %d\n", round);
        for (int i = 1; i <= N; i) {
            while(shared_memory[0] != 999) { // Wait for the i-th child to finish
                printf("Parent waiting \n");
                sleep(1);
            }
            printf("Parent got %d: %d\n", children_pids[i-1], shared_memory[i]);
            i++;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) usage(argv[0]);

    int N = atoi(argv[1]);
    int M = atoi(argv[2]);

    srand(time(NULL));

    int shm_fd = shm_open("/game", O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) ERR("shm_open");

    if (ftruncate(shm_fd, N * sizeof(int)) == -1) ERR("ftruncate");

    int* shared_memory = mmap(NULL, N * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_memory == MAP_FAILED) ERR("mmap");
    int children_pids[N];

    for (int i = 1; i <= N; i++) {
        switch (children_pids[i-1]=fork()) {
            case -1:
                ERR("fork");
            case 0:
                childwork(N, M, shared_memory, i);
            default:
                break;
        }
    }

    parent_work(N, M, shared_memory, children_pids);

    if (munmap(shared_memory, N * sizeof(int)) == -1) ERR("munmap");
    if (shm_unlink("/game") == -1) ERR("shm_unlink");

    return 0;
}
