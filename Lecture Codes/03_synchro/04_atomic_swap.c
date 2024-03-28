#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <string.h>
#include <pthread.h>

#define ITERATIONS 10000000

atomic_int lck = 0;

/* gcc -02
lock:
        mov     eax, 1
.L2:
        xchg    eax, DWORD PTR lck[rip]
        test    eax, eax
        jne     .L2
        ret
 */
void lock(void) {
    int key = 1;
    do {
        key = atomic_exchange(&lck, key);
    } while(key);
}

/* gcc -02
unlock:
        xor     eax, eax
        xchg    eax, DWORD PTR lck[rip]
        ret
 */
void unlock(void) {
    lck = 0;
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
        unlock();
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