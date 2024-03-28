#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <semaphore.h>

#define BUFFER_SIZE 10
#define ITERATIONS 10000000

typedef struct context {
    sem_t mtx;
    sem_t empty;
    sem_t nonempty;

    int items[BUFFER_SIZE];
    int in;
    int out;
} context_t;

#define SHM_SIZE sizeof(context_t)

void producer(context_t *ctx) {
    for (int i = 0; i < ITERATIONS; ++i) {
        sem_wait(&ctx->empty);
        sem_wait(&ctx->mtx);

        int new_item = i;
        ctx->items[ctx->in] = new_item;
        ctx->in = (ctx->in + 1) % BUFFER_SIZE;

        sem_post(&ctx->mtx);
        sem_post(&ctx->nonempty);
    }
}

void consumer(context_t *ctx) {
    for (int i = 0; i < ITERATIONS; ++i) {
        sem_wait(&ctx->nonempty);
        sem_wait(&ctx->mtx);

        int item = ctx->items[ctx->out];
        ctx->out = (ctx->out + 1) % BUFFER_SIZE;

        if (item != i) {
            fprintf(stderr, "item corruption!\n");
        }

        sem_post(&ctx->mtx);
        sem_post(&ctx->empty);
    }
}


int main() {

    srand(getpid());

    shm_unlink("/sop_shm");

    int fd = shm_open("/sop_shm", O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd < 0) {
        perror("shm_open()");
        return 1;
    }

    if (ftruncate(fd, SHM_SIZE) < 0) {
        perror("ftruncate()");
        return 1;
    }

    void *ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap()");
        return 1;
    }

    close(fd);

    context_t *ctx = (context_t *) ptr;
    if (sem_init(&ctx->mtx, 1, 1) < 0) {
        perror("sem_init()");
        return 1;
    }
    if (sem_init(&ctx->empty, 1, BUFFER_SIZE) < 0) {
        perror("sem_init()");
        return 1;
    }
    if (sem_init(&ctx->nonempty, 1, 0) < 0) {
        perror("sem_init()");
        return 1;
    }

    switch (fork()) {
        case -1:
            perror("fork()");
            return 1;
        case 0: {
            // child
            consumer(ctx);
            break;
        }
        default: {
            // parent
            producer(ctx);
            while (wait(NULL) > 0);
            break;
        }
    }

    printf("Unmapping file\n");
    sem_destroy(&ctx->mtx);
    sem_destroy(&ctx->empty);
    sem_destroy(&ctx->nonempty);
    munmap(ptr, SHM_SIZE);
    return 0;
}
