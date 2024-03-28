#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEFAULTNUMBEROFTHREADS 3
#define DEFAULTSIZEOFARRAY 10

typedef unsigned int UINT;
typedef struct argsChildren
{
  pthread_t tid;
  UINT seed;
  int index;
  int SizeofArray;
  int NumberofThreads;
  int No;
} argsChildren_t;

int *taskArray;
float *resultArray;
pthread_mutex_t *mutexes;
int *calculatedCells;

void ReadArguments(int argc, char *argv[], int *NumberofThreads, int *SizeofArray)
{
  *NumberofThreads = DEFAULTNUMBEROFTHREADS;
  *SizeofArray = DEFAULTSIZEOFARRAY;
  if (argc == 2)
  {
    *NumberofThreads = atoi(argv[1]);
  }
  if (argc == 3)
  {
    *NumberofThreads = atoi(argv[1]);
    *SizeofArray = atoi(argv[2]);
  }
}

void *child_thread(void *arg)
{
  argsChildren_t *args = arg;
  int index;
  while (1)
  {
    index = rand_r(&args->seed) % args->SizeofArray;
    printf("Randomized index %d\n", index);
    pthread_mutex_lock(&mutexes[index]);
    int i;
    for( i = 0; i < args->SizeofArray; i++)
    {
      if (calculatedCells[index] == 0){
        break;
      }
      else{
        pthread_mutex_unlock(&mutexes[index]);
        index = (index+i)% args->SizeofArray;
        pthread_mutex_lock(&mutexes[index]);
      }
    }
    if(i== args->SizeofArray){
      pthread_mutex_unlock(&mutexes[index]);
      printf("All cells are calculated\n");
      break;
    }
    calculatedCells[index] = -1;

    resultArray[index] = sqrt(taskArray[index]);
    printf("\nsqrt(%d) = %f\n", taskArray[index], sqrt(taskArray[index]));

    pthread_mutex_unlock(&mutexes[index]);

    // Sleep outside the critical section
    sleep(1);
  }

  return NULL;
}

void make_children(argsChildren_t *argsArray, int NumberofThreads, int SizeofArray)
{
  taskArray = malloc(SizeofArray * sizeof(int));
  resultArray = malloc(SizeofArray * sizeof(float));
  calculatedCells = malloc(SizeofArray * sizeof(int));
  mutexes = malloc(SizeofArray * sizeof(pthread_mutex_t));

  for (int i = 0; i < SizeofArray; i++)
  {
    pthread_mutex_init(&mutexes[i], NULL);
    taskArray[i] = rand() % 60;
    calculatedCells[i] = 0;
  }

  for (int i = 0; i < NumberofThreads; i++)
  {
    argsArray[i].seed = (UINT)rand();
    argsArray[i].SizeofArray = SizeofArray;
    argsArray[i].NumberofThreads = NumberofThreads;
    argsArray[i].No = i;

    if (pthread_create(&argsArray[i].tid, NULL, child_thread, &argsArray[i]) != 0)
      perror("Couldn't create thread");
  }
}

void wait_children(argsChildren_t *argsArray, int NumberofThreads)
{
  for (int i = 0; i < NumberofThreads; i++)
  {
    pthread_join(argsArray[i].tid, NULL);
  }
}

int main(int argc, char *argv[])
{
  int NumberofThreads = 0;
  int SizeofArray = 0;
  srand(time(NULL));
  ReadArguments(argc, argv, &NumberofThreads, &SizeofArray);
  argsChildren_t *argsArray = malloc(NumberofThreads * sizeof(argsChildren_t));
  make_children(argsArray, NumberofThreads, SizeofArray);
  wait_children(argsArray, NumberofThreads);

  printf("\nMain thread is out\n");
  for (int i = 0; i < SizeofArray; i++)
  {
    printf("%f ", resultArray[i]);
  }
  printf("\n");
  for (int i = 0; i < SizeofArray; i++)
  {
    printf("%d ", taskArray[i]);
  }
  printf("\n");

  free(taskArray);
  free(resultArray);
  free(argsArray);
  free(calculatedCells);
  free(mutexes);
  return 0;
}
