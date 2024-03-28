#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define ITERATIONS 10000000

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

void lock(void) {
    pthread_mutex_lock(&mtx);
}

void unlock(void) {
    pthread_mutex_unlock(&mtx);
}

void *producer(void *arg) {
    int *counter = (int *) arg;

    for (int i = 0; i < ITERATIONS; ++i) {
        lock();
        (*counter)++;
        unlock();
    }

    return NULL;
}

void *consumer(void *arg) {
    int *counter = (int *) arg;

    for (int i = 0; i < ITERATIONS; ++i) {
        lock();
        (*counter)--;
        // getchar(); // TODO: Uncomment and run program under strace
        unlock();
    }

    return NULL;
}

int main() {

    srand(getpid());

    int counter = 0;

    printf("Creating producer\n");

    pthread_t producer_tid, consumer_tid;
    int ret;
    if ((ret = pthread_create(&producer_tid, NULL, producer, &counter)) != 0) {
        fprintf(stderr, "pthread_create(): %s", strerror(ret));
        return 1;
    }

    printf("Creating consumer\n");

    if ((ret = pthread_create(&consumer_tid, NULL, consumer, &counter)) != 0) {
        fprintf(stderr, "pthread_create(): %s", strerror(ret));
        return 1;
    }

    printf("Created both threads\n");

    pthread_join(producer_tid, NULL);
    pthread_join(consumer_tid, NULL);

    printf("counter = %d\n", counter);

    return 0;
}