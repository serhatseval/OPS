#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define DEFAULT_THREADCOUNT 10
#define DEFAULT_SAMPLESIZE 100

typedef unsigned int UINT;

// Structure to hold thread arguments
typedef struct argsEstimation {
    pthread_t tid;     // Thread ID
    UINT seed;         // Seed for random number generation
    int samplesCount;  // Number of samples for each thread
} argsEstimation_t;

// Function to read command-line arguments
void ReadArguments(int argc, char **argv, int *threadCount, int *samplesCount);

// Function to estimate Pi using Monte Carlo simulation in a thread
void *pi_estimation(void *args);

int main(int argc, char **argv) {
    int threadCount, samplesCount;
    double *subresult;

    // Read command-line arguments or use default values
    ReadArguments(argc, argv, &threadCount, &samplesCount);

    // Allocate memory for thread arguments
    argsEstimation_t *estimations = (argsEstimation_t *)malloc(sizeof(argsEstimation_t) * threadCount);
    if (estimations == NULL) {
        perror("Memory allocation error");
        exit(EXIT_FAILURE);
    }

    // Initialize thread arguments with random seeds and sample counts
    srand(time(NULL));
    for (int i = 0; i < threadCount; i++) {
        estimations[i].seed = rand();
        estimations[i].samplesCount = samplesCount;
    }

    // Create threads for Pi estimation
    for (int i = 0; i < threadCount; i++) {
        int err = pthread_create(&(estimations[i].tid), NULL, pi_estimation, &estimations[i]);
        if (err != 0) {
            perror("Thread creation error");
            exit(EXIT_FAILURE);
        }
    }

    double cumulativeResult = 0.0;

    // Wait for threads to finish and accumulate results
    for (int i = 0; i < threadCount; i++) {
        int err = pthread_join(estimations[i].tid, (void *)&subresult);
        if (err != 0) {
            perror("Thread join error");
            exit(EXIT_FAILURE);
        }

        // If the result is not NULL, add it to the cumulative result and free the memory
        if (NULL != subresult) {
            cumulativeResult += *subresult;
            free(subresult);
        }
    }

    // Calculate the final result by averaging the thread results
    double result = cumulativeResult / threadCount;
    printf("Estimated Pi: %f\n", result);

    // Free allocated memory for thread arguments
    free(estimations);

    return EXIT_SUCCESS;
}

// Function to read command-line arguments and set default values
void ReadArguments(int argc, char **argv, int *threadCount, int *samplesCount) {
    *threadCount = DEFAULT_THREADCOUNT;
    *samplesCount = DEFAULT_SAMPLESIZE;

    if (argc >= 2) {
        *threadCount = atoi(argv[1]);
        if (*threadCount <= 0) {
            printf("Invalid value for 'threadCount'");
            exit(EXIT_FAILURE);
        }
    }

    if (argc >= 3) {
        *samplesCount = atoi(argv[2]);
        if (*samplesCount <= 0) {
            printf("Invalid value for 'samplesCount'");
            exit(EXIT_FAILURE);
        }
    }
}

// Function to estimate Pi using Monte Carlo simulation in a thread
void *pi_estimation(void *voidPtr) {
    argsEstimation_t *args = voidPtr;
    double *result;

    // Allocate memory for the result
    if (NULL == (result = malloc(sizeof(double)))) {
        perror("Memory allocation error");
        exit(EXIT_FAILURE);
    }

    int insideCount = 0;

    // Perform Monte Carlo simulation to estimate Pi
    for (int i = 0; i < args->samplesCount; i++) {
        double x = ((double)rand_r(&args->seed) / (double)RAND_MAX);
        double y = ((double)rand_r(&args->seed) / (double)RAND_MAX);

        if (sqrt(x * x + y * y) <= 1.0) {
            insideCount++;
        }
    }

    // Calculate the thread's result and store it in the allocated memory
    *result = 4.0 * (double)insideCount / (double)args->samplesCount;

    // Return the result
    return result;
}
