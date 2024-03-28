#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#define N 10
#define ITERATIONS 10000000

sem_t mtx;
sem_t empty;
sem_t nonempty;

typedef struct bounded_buffer {
    int items[N];
    int in;
    int out;
} bounded_buffer_t;

void *producer(void *arg) {
    bounded_buffer_t *buffer = (bounded_buffer_t *) arg;

    for (int i = 0; i < ITERATIONS; ++i) {
        sem_wait(&empty);
        sem_wait(&mtx);

        int new_item = i;
        buffer->items[buffer->in] = new_item;
        buffer->in = (buffer->in + 1) % N;

        sem_post(&mtx);
        sem_post(&nonempty);
    }

    return NULL;
}

void *consumer(void *arg) {
    bounded_buffer_t *buffer = (bounded_buffer_t *) arg;

    for (int i = 0; i < ITERATIONS; ++i) {
        sem_wait(&nonempty);
        sem_wait(&mtx);

        int item = buffer->items[buffer->out];
        buffer->out = (buffer->out + 1) % N;

        if (item != i) {
            fprintf(stderr, "item corruption!\n");
        }

        sem_post(&mtx);
        sem_post(&empty);
    }

    return NULL;
}

int main() {

    bounded_buffer_t buffer = {};

    sem_init(&mtx, 0, 1);
    sem_init(&empty, 0, N);
    sem_init(&nonempty, 0, 0);

    srand(getpid());

    printf("Creating producer\n");

    pthread_t producer_tid, consumer_tid;
    int ret;
    if ((ret = pthread_create(&producer_tid, NULL, producer, &buffer)) != 0) {
        fprintf(stderr, "pthread_create(): %s", strerror(ret));
        return 1;
    }

    printf("Creating consumer\n");

    if ((ret = pthread_create(&consumer_tid, NULL, consumer, &buffer)) != 0) {
        fprintf(stderr, "pthread_create(): %s", strerror(ret));
        return 1;
    }

    printf("Created both threads\n");

    pthread_join(producer_tid, NULL);
    pthread_join(consumer_tid, NULL);

    return 0;
}
