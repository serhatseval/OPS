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

    for (int i = 0; i < N; i++) {
        switch (fork()) {
            case -1:
                ERR("fork");
            case 0:
                srand(time(NULL) ^ (getpid()));
                int number = rand() % M+1;
                printf("Child %d: %d\n", i, number);
                shared_memory[i] = number;
            default:
                break;
        }
    }

    for (int round = 0; round < M; round++) {
    shared_memory[0] = round;  // Use the first element of the shared memory as the "new round" message
    for (int i = 0; i < N; i++) {
        wait(NULL);  // Wait for a player to send a card
    }
    // TODO: Determine the winner of the round
}

    if (munmap(shared_memory, N * sizeof(int)) == -1) ERR("munmap");
    if (shm_unlink("/game") == -1) ERR("shm_unlink");

    return 0;
}
