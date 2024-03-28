#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#define N 10
#define ITERATIONS 10000000

int counter = 0;

sem_t mtx;
int readers_count;
sem_t rw_mtx;

void *writer(void *arg) {
    for (int i = 0; i < ITERATIONS; ++i) {
        sem_wait(&rw_mtx);

        printf("writing (%d)...\n", i);
        counter++;

        sem_post(&rw_mtx);
    }

    return NULL;
}

void *reader(void *arg) {
    for (int i = 0; i < ITERATIONS; ++i) {

        while (rand() % 100); // Slow down readers a bit

        sem_wait(&mtx);
        readers_count++;
        if (readers_count == 1) {
            sem_wait(&rw_mtx);
        }
        sem_post(&mtx);

        printf("reading (%lu)...\n", pthread_self());

        sem_wait(&mtx);
        readers_count--;
        if (readers_count == 0)
            sem_post(&rw_mtx);
        sem_post(&mtx);
    }

    return NULL;
}

int main() {

    srand(getpid());

    sem_init(&mtx, 0, 1);
    sem_init(&rw_mtx, 0, 1);

    printf("Creating writer\n");

    pthread_t writer_tid, reader1_tid, reader2_tid;
    int ret;
    if ((ret = pthread_create(&writer_tid, NULL, writer, NULL)) != 0) {
        fprintf(stderr, "pthread_create(): %s", strerror(ret));
        return 1;
    }

    printf("Creating reader 1\n");

    if ((ret = pthread_create(&reader1_tid, NULL, reader, NULL)) != 0) {
        fprintf(stderr, "pthread_create(): %s", strerror(ret));
        return 1;
    }

    printf("Creating reader 2\n");

    if ((ret = pthread_create(&reader2_tid, NULL, reader, NULL)) != 0) {
        fprintf(stderr, "pthread_create(): %s", strerror(ret));
        return 1;
    }

    printf("Created both threads\n");

    pthread_join(writer_tid, NULL);
    pthread_join(reader1_tid, NULL);
    pthread_join(reader2_tid, NULL);

    return 0;
}
