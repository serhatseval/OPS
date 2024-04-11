// #include <bits/pthreadtypes.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "macros.h"

// Use those names to cooperate with $make clean-resources target.
#define SHMEM_SEMAPHORE_NAME "/sop-shmem-sem"
#define SHMEM_NAME "/sop-shmem"

typedef struct shared_mem
{
    uint64_t workers;
    uint64_t tried;
    uint64_t hit;
    pthread_mutex_t lock;
    float a;
    float b;
} shared_mem;

// Values of this function is in range (0,1]
double func(double x)
{
    usleep(2000);
    return exp(-x * x);
}

/**
 * It counts hit points by Monte Carlo method.
 * Use it to process one batch of computation.
 * @param N Number of points to randomize
 * @param a Lower bound of integration
 * @param b Upper bound of integration
 * @return Number of points which was hit.
 */
int randomize_points(int N, float a, float b)
{
    int i = 0;
    for (; i < N; ++i)
    {
        double rand_x = ((double)rand() / RAND_MAX) * (b - a) + a;
        double rand_y = ((double)rand() / RAND_MAX);
        double real_y = func(rand_x);

        if (rand_y <= real_y)
            i++;
    }
    return i;
}

/**
 * This function calculates approxiamtion of integral from counters of hit and total points.
 * @param total_randomized_points Number of total randomized points.
 * @param hit_points NUmber of hit points.
 * @param a Lower bound of integration
 * @param b Upper bound of integration
 * @return The approximation of intergal
 */
double summarize_calculations(uint64_t total_randomized_points, uint64_t hit_points, float a, float b)
{
    return (b - a) * ((double)hit_points / (double)total_randomized_points);
}

/**
 * This function locks mutex and can sometime die (it has 2% chance to die).
 * It cannot die if lock would return an error.
 * It doesn't handle any errors. It's users responsibility.
 * Use it only in STAGE 4.
 *
 * @param mtx Mutex to lock
 * @return Value returned from pthread_mutex_lock.
 */
int random_death_lock(pthread_mutex_t *mtx)
{
    int ret = pthread_mutex_lock(mtx);
    if (ret)
        return ret;

    // 2% chance to die
    if (rand() % 50)
        abort();
    return ret;
}

void usage(char *argv[])
{
    printf("%s a b N - calculating integral with multiple processes\n", argv[0]);
    printf("a - Start of segment for integral (default: -1)\n");
    printf("b - End of segment for integral (default: 1)\n");
    printf("N - Size of batch to calculate before rport to shared memory (default: 1000)\n");
}

shared_mem *mem_init(float a, float b)
{
    int shmem_fd;
    /* open semaphore for synchronization of shared memory initialization*/
    sem_t *shmem_init_sem;
    shmem_init_sem = sem_open(SHMEM_SEMAPHORE_NAME, O_CREAT, S_IRUSR | S_IWUSR, 1);
    if (shmem_init_sem == SEM_FAILED)
        ERR("sem_open");

    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        ERR("clock_gettime");

    ts.tv_sec += 10;
    int s = 0;
    while ((s = sem_timedwait(shmem_init_sem, &ts)) == -1 && errno == EINTR)
        continue; /* Restart if interrupted by handler */

    /* Check what happened */
    if (s == -1)
    {
        if (errno == ETIMEDOUT)
            ERR("sem_timedwait() timed out\n");
        else
            ERR("sem_timedwait");
    }

    /* Try to create shared memory */
    int initialize = 1;
    shmem_fd = shm_open(SHMEM_NAME, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    if (shmem_fd == -1)
    {
        if (errno != EEXIST)
        {
            sem_post(shmem_init_sem);
            ERR("shm_open");
        }

        /* Open existing shmem */
        shmem_fd = shm_open(SHMEM_NAME, O_RDWR, S_IRUSR | S_IWUSR);
        if (shmem_fd == -1)
        {
            sem_post(shmem_init_sem);
            ERR("shm_open");
        }
        initialize = 0;
    }

    if (initialize && ftruncate(shmem_fd, sizeof(shared_mem)) == -1)
    {
        // Destroy to prevent partially created shmem
        shm_unlink(SHMEM_NAME);
        sem_post(shmem_init_sem);
        ERR("ftruncate");
    }

    shared_mem *shmem = mmap(NULL, sizeof(shared_mem), PROT_READ | PROT_WRITE, MAP_SHARED, shmem_fd, 0);
    if (shmem == MAP_FAILED)
    {
        if (initialize)  // Destroy to prevent partially created shmem
            shm_unlink(SHMEM_NAME);
        sem_post(shmem_init_sem);
        ERR("mmap");
    }

    close(shmem_fd);

    if (initialize)
    {
        shmem->workers = 1;
        shmem->tried = 0;
        shmem->hit = 0;
        shmem->a = a;
        shmem->b = b;

        errno = 0;

        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&shmem->lock, &attr);

        if (errno)  // Something went wrong => destroying shmem
        {
            // Destroy to prevent partially created shmem
            shm_unlink(SHMEM_NAME);
            sem_post(shmem_init_sem);
            ERR("shmem init");
        }
    }
    else
    {
        if (a != shmem->a || b != shmem->b)
            ERR("Inconsistent ranges");
        // Lock counter, because someone can be leaving at the moment
        pthread_mutex_lock(&shmem->lock);
        shmem->workers++;
        pthread_mutex_unlock(&shmem->lock);
    }
    sem_post(shmem_init_sem);

    if (sem_close(shmem_init_sem))
        ERR("sem_close");

    return shmem;
}

void mem_close(shared_mem *shmem) { munmap(shmem, sizeof(shared_mem)); }

void mem_destroy(shared_mem *shmem)
{
    pthread_mutex_destroy(&shmem->lock);

    mem_close(shmem);

    if (shm_unlink(SHMEM_NAME))
        ERR("shm_unlink");
    if (sem_unlink(SHMEM_SEMAPHORE_NAME))
        ERR("sem_unlink");
}

void finish(shared_mem *shmem)
{
    sem_t *shmem_init_sem;
    shmem_init_sem = sem_open(SHMEM_SEMAPHORE_NAME, O_CREAT, S_IRUSR | S_IWUSR, 1);
    if (shmem_init_sem == SEM_FAILED)
        ERR("sem_open");

    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        ERR("clock_gettime");

    ts.tv_sec += 10;
    int s = 0;
    while ((s = sem_timedwait(shmem_init_sem, &ts)) == -1 && errno == EINTR)
        continue; /* Restart if interrupted by handler */

    /* Check what happened */
    if (s == -1)
    {
        if (errno == ETIMEDOUT)
            ERR("sem_timedwait() timed out\n");
        else
            ERR("sem_timedwait");
    }

    if (--shmem->workers == 0)
    {
        pthread_mutex_lock(&shmem->lock);
        printf("\nResult : %f\n", (double)shmem->hit / (double)shmem->tried);
        pthread_mutex_unlock(&shmem->lock);

        mem_destroy(shmem);
    }
    else
    {
        mem_close(shmem);
    }

    sem_post(shmem_init_sem);
    sem_close(shmem_init_sem);
}

void work(float a, float b, int N, int t, shared_mem *shmem)
{
    while (t--)
    {
        int res = randomize_points(N, a, b);

        pthread_mutex_lock(&shmem->lock);

        shmem->hit += res;
        shmem->tried += N;

        printf("Partial result : hit - %ld, tried - %ld\n", shmem->hit, shmem->tried);

        pthread_mutex_unlock(&shmem->lock);
    }
}

int main(int argc, char *argv[])
{
    float a = -1, b = 1;
    int N = 1000;
    switch (argc)
    {
        case 4:
            N = atoi(argv[3]);
            b = atof(argv[2]);
            a = atof(argv[1]);
            break;
        case 3:
            b = atof(argv[2]);
            a = atof(argv[1]);
            break;
        case 2:
            a = atof(argv[1]);
            break;
    }

    if (N < 0 || !(a < b))
        usage(argv);

    shared_mem *shmem = mem_init(a, b);

    work(a, b, N, 3, shmem);

    finish(shmem);

    printf("[%d] Returning\n", getpid());

    return EXIT_SUCCESS;
}
