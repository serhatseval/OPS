#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <string.h>
#include <pthread.h>

#define ITERATIONS 10000000

atomic_int flag[2] = {0, 0};
atomic_int turn = 0;

void lock(int i) {
    int j = 1 - i;
    flag[i] = 1;
    turn = j;
    while (flag[j] && turn == j);
}

void unlock(int i) {
    flag[i] = 0;
}

void *producer(void *arg) {
    int *counter = (int *) arg;

    for (int i = 0; i < ITERATIONS; ++i) {
        lock(0);
        (*counter)++;
        unlock(0);
    }

    return NULL;
}

void *consumer(void *arg) {
    int *counter = (int *) arg;

    for (int i = 0; i < ITERATIONS; ++i) {
        lock(1);
        (*counter)--;
        unlock(1);
    }

    return NULL;
}

int main() {

    srand(getpid());

    int counter = 0;

    pthread_t producer_tid, consumer_tid;
    int ret;
    if ((ret = pthread_create(&producer_tid, NULL, producer, &counter)) != 0) {
        fprintf(stderr, "pthread_create(): %s", strerror(ret));
        return 1;
    }

    if ((ret = pthread_create(&consumer_tid, NULL, consumer, &counter)) != 0) {
        fprintf(stderr, "pthread_create(): %s", strerror(ret));
        return 1;
    }

    pthread_join(producer_tid, NULL);
    pthread_join(consumer_tid, NULL);

    printf("counter = %d\n", counter);

    return 0;
}