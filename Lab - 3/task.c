#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

pthread_mutex_t vector_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t guessed_value_mutex = PTHREAD_MUTEX_INITIALIZER;

#define DEFAULT_NUMBER_OF_THREADS 3
#define DEFAULT_SIZE_OF_VECTOR 10
#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

typedef unsigned int UINT;

typedef struct Vector
{
    int *array;
    int size;
} Vector_t;

typedef struct args
{
    UINT seed;
    pthread_t tid;
} args_t;

Vector_t vector;
int guessed_value = 0;
int SigIntReceived = 0;

void sethandler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    act.sa_flags = SA_RESTART; // Add this line

    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void sigInt_handler(int sig)
{
    SigIntReceived = 1;
}

void ReadArguments(int argc, char **argv, int *SizeofVector, int *NumberofThreads)
{
    *NumberofThreads = DEFAULT_NUMBER_OF_THREADS;
    *SizeofVector = DEFAULT_SIZE_OF_VECTOR;
    if (argc == 3)
    {
        *NumberofThreads = atoi(argv[1]);
        *SizeofVector = atoi(argv[2]);
    }
}

void *workthread(void *arg)
{
    uint64_t tid;
    pthread_threadid_np(NULL, &tid);
    printf("Thread id : %llu\n", tid);

    while (1)
    {
        pthread_mutex_lock(&vector_mutex);
        pthread_mutex_lock(&guessed_value_mutex);

        int randomIndex = rand() % vector.size;

        if (vector.array[randomIndex] == 0)
        {
            // Do nothing
        }
        else if (vector.array[randomIndex] == guessed_value)
        {
            // Handle SIGUSR2 (not implemented)
        }
        else if (vector.array[randomIndex] > 0)
        {
            guessed_value = vector.array[randomIndex];
            fflush(stdout);
        }

        pthread_mutex_unlock(&guessed_value_mutex);
        pthread_mutex_unlock(&vector_mutex);
    
    }

    return NULL;
}

void CreateThreads(args_t *threads, int NumberofThreads)
{
    for (int i = 0; i < NumberofThreads; i++)
    {
        unsigned int seed = time(NULL) + getpid() + i;
        threads[i].seed = seed;
    }

    for (int i = 0; i < NumberofThreads; i++)
    {
        if (pthread_create(&threads[i].tid, NULL, workthread, &threads[i]))
        {
            printf("Error creating thread\n");
            exit(EXIT_FAILURE);
        }
    }

}

void JoinThreads(args_t *threads, int NumberofThreads)
{
    for (int i = 0; i < NumberofThreads; i++)
    {
        if (pthread_join(threads[i].tid, NULL))
        {
            printf("Error joining thread\n");
            exit(EXIT_FAILURE);
        }
    }
}

void print_vector()
{
    pthread_mutex_lock(&vector_mutex);
    for (int j = 0; j < vector.size; j++)
    {
        printf("%d ", vector.array[j]);
    }
    printf("\n");
    fflush(stdout);
    pthread_mutex_unlock(&vector_mutex);
}

void prepare_vector(int SizeofVector)
{
    vector.array = malloc(SizeofVector * sizeof(int));
    vector.size = SizeofVector;

    if (!vector.array)
    {
        perror("Couldn't allocate memory for vector.array");
    }
    for (int i = 0; i < SizeofVector; i++)
    {
        vector.array[i] = 0;
    }
}

void mainWork(sigset_t *oldMask)
{   
    while (sigsuspend(oldMask))
    {
        if (SigIntReceived)
        {
            printf("Received SIGINT\n");
            int randomIndex = rand() % vector.size;
            int randomValue = rand() % 254;
            pthread_mutex_lock(&vector_mutex);
            vector.array[randomIndex] = randomValue + 1;
            pthread_mutex_unlock(&vector_mutex);
            SigIntReceived = 0;
        }

        print_vector();
        // sleep(1);
    }
}

int main(int argc, char **argv)
{
    printf("PID: %d\n", getpid());
    srand(time(NULL));
    int SizeofVector;
    int NumberofThreads;
    ReadArguments(argc, argv, &SizeofVector, &NumberofThreads);

    prepare_vector(SizeofVector);

    args_t *args = malloc(NumberofThreads * sizeof(args_t));
    if (!args)
    {
        perror("Couldn't allocate memory for args");
        return EXIT_FAILURE;
    }
    pthread_mutex_init(&vector_mutex, NULL);
    pthread_mutex_init(&guessed_value_mutex, NULL);

    sethandler(sigInt_handler, SIGINT);
    sigset_t oldMask, newMask;
    sigemptyset(&newMask);
    sigaddset(&newMask, SIGINT);
    if (pthread_sigmask(SIG_BLOCK, &newMask, &oldMask) != 0)
        ERR("SIG_BLOCK error");

    CreateThreads(args, NumberofThreads);
    mainWork(&oldMask);

    JoinThreads(args, NumberofThreads);
    pthread_mutex_destroy(&vector_mutex);
    pthread_mutex_destroy(&guessed_value_mutex);

    free(vector.array);
    free(args);

    return EXIT_SUCCESS;
}
