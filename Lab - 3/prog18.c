#include <pthread.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAXLINE 4096
#define DEFAULT_N 1000
#define DEFAULT_K 10
#define BIN_COUNT 11
#define NEXT_DOUBLE(seedptr) ((double)rand_r(seedptr) / (double)RAND_MAX)
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

// Define an alias for unsigned int
typedef unsigned int UINT;

// Define a structure to hold the arguments for each thrower thread
typedef struct argsThrower
{
    pthread_t tid;               // Thread ID
    UINT seed;                    // Seed for random number generation
    int *pBallsThrown;           // Pointer to the count of balls thrown
    int *pBallsWaiting;          // Pointer to the count of balls waiting to be thrown
    int *bins;                    // Pointer to the array of bins
    pthread_mutex_t *mxBins;     // Pointer to an array of mutexes for bins
    pthread_mutex_t *pmxBallsThrown;  // Pointer to the mutex for the count of balls thrown
    pthread_mutex_t *pmxBallsWaiting; // Pointer to the mutex for the count of balls waiting
} argsThrower_t;

// Function to read command-line arguments
void ReadArguments(int argc, char **argv, int *ballsCount, int *throwersCount);

// Function to create thrower threads
void make_throwers(argsThrower_t *argsArray, int throwersCount);

// Function representing the behavior of a thrower thread
void *throwing_func(void *args);

// Function to simulate throwing a ball and return the bin number
int throwBall(UINT *seedptr);

int main(int argc, char **argv)
{
    int ballsCount, throwersCount;
    ReadArguments(argc, argv, &ballsCount, &throwersCount);
    int ballsThrown = 0, bt = 0;
    int ballsWaiting = ballsCount;

    // Mutex to control access to the count of balls thrown
    pthread_mutex_t mxBallsThrown = PTHREAD_MUTEX_INITIALIZER;

    // Mutex to control access to the count of balls waiting
    pthread_mutex_t mxBallsWaiting = PTHREAD_MUTEX_INITIALIZER;

    // Array to represent the bins
    int bins[BIN_COUNT];

    // Array of mutexes to control access to each bin
    pthread_mutex_t mxBins[BIN_COUNT];

    // Initialize bins and corresponding mutexes
    for (int i = 0; i < BIN_COUNT; i++)
    {
        bins[i] = 0;
        if (pthread_mutex_init(&mxBins[i], NULL))
            ERR("Couldn't initialize mutex!");
    }

    // Allocate memory for thrower arguments
    argsThrower_t *args = (argsThrower_t *)malloc(sizeof(argsThrower_t) * throwersCount);
    if (args == NULL)
        ERR("Malloc error for throwers arguments!");

    // Seed the random number generator
    srand(time(NULL));

    // Initialize thrower arguments
    for (int i = 0; i < throwersCount; i++)
    {
        args[i].seed = (UINT)rand();
        args[i].pBallsThrown = &ballsThrown;
        args[i].pBallsWaiting = &ballsWaiting;
        args[i].bins = bins;
        args[i].pmxBallsThrown = &mxBallsThrown;
        args[i].pmxBallsWaiting = &mxBallsWaiting;
        args[i].mxBins = mxBins;
    }

    // Create thrower threads
    make_throwers(args, throwersCount);

    // Wait for all balls to be thrown
    while (bt < ballsCount)
    {
        sleep(1);
        pthread_mutex_lock(&mxBallsThrown);
        bt = ballsThrown;
        pthread_mutex_unlock(&mxBallsThrown);
    }

    // Calculate and print statistics
    int realBallsCount = 0;
    double meanValue = 0.0;
    for (int i = 0; i < BIN_COUNT; i++)
    {
        realBallsCount += bins[i];
        meanValue += bins[i] * i;
    }
    meanValue = meanValue / realBallsCount;

    printf("Bins count:\n");
    for (int i = 0; i < BIN_COUNT; i++)
        printf("%d\t", bins[i]);
    printf("\nTotal balls count : %d\nMean value: %f\n", realBallsCount, meanValue);

    // Exit the program
    exit(EXIT_SUCCESS);
}

// Function to read command-line arguments
void ReadArguments(int argc, char **argv, int *ballsCount, int *throwersCount)
{
    *ballsCount = DEFAULT_N;
    *throwersCount = DEFAULT_K;

    if (argc >= 2)
    {
        *ballsCount = atoi(argv[1]);
        if (*ballsCount <= 0)
        {
            printf("Invalid value for 'balls count'");
            exit(EXIT_FAILURE);
        }
    }

    if (argc >= 3)
    {
        *throwersCount = atoi(argv[2]);
        if (*throwersCount <= 0)
        {
            printf("Invalid value for 'throwers count'");
            exit(EXIT_FAILURE);
        }
    }
}

// Function to create thrower threads
void make_throwers(argsThrower_t *argsArray, int throwersCount)
{
    pthread_attr_t threadAttr;

    // Initialize thread attributes
    if (pthread_attr_init(&threadAttr))
        ERR("Couldn't create pthread_attr_t");

    // Set thread attributes to be detached
    if (pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED))
        ERR("Couldn't setdetachsatate on pthread_attr_t");

    // Create thrower threads
    for (int i = 0; i < throwersCount; i++)
    {
        if (pthread_create(&argsArray[i].tid, &threadAttr, throwing_func, &argsArray[i]))
            ERR("Couldn't create thread");
    }

    // Destroy thread attributes
    pthread_attr_destroy(&threadAttr);
}

// Function representing the behavior of a thrower thread
void *throwing_func(void *voidArgs)
{
    argsThrower_t *args = voidArgs;

    // Infinite loop representing the continuous throwing of balls
    while (1)
    {
        pthread_mutex_lock(args->pmxBallsWaiting);
        if (*args->pBallsWaiting > 0)
        {
            (*args->pBallsWaiting) -= 1;
            pthread_mutex_unlock(args->pmxBallsWaiting);
        }
        else
        {
            pthread_mutex_unlock(args->pmxBallsWaiting);
            break; // Exit the loop when there are no more balls to throw
        }

        // Simulate throwing a ball and determine the bin
        int binno = throwBall(&args->seed);

        // Lock the mutex for the corresponding bin
        pthread_mutex_lock(&args->mxBins[binno]);

        // Update the count of balls in the bin
        args->bins[binno] += 1;

        // Unlock the mutex for the bin
        pthread_mutex_unlock(&args->mxBins[binno]);

        // Lock the mutex for the count of balls thrown
        pthread_mutex_lock(args->pmxBallsThrown);

        // Update the total count of balls thrown
        (*args->pBallsThrown) += 1;

        // Unlock the mutex for the count of balls thrown
        pthread_mutex_unlock(args->pmxBallsThrown);
    }

    return NULL;
}

/* returns # of bin where ball has landed */
int throwBall(UINT *seedptr)
{
    int result = 0;

    // Simulate throwing the ball and determining the bin based on a random number
    for (int i = 0; i < BIN_COUNT - 1; i++)
        if (NEXT_DOUBLE(seedptr) > 0.5)
            result++;

    return result;
}
