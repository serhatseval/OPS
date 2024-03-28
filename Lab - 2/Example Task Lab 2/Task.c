#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) \
  do { \
    fprintf(stderr, "%s:%d\n", __FILE__, __LINE__); \
    perror(source); \
    kill(0, SIGKILL); \
    exit(EXIT_FAILURE); \
  } while (0)

void sethandler(void (*f)(int), int sigNo)
{
  struct sigaction act;
  memset(&act, 0, sizeof(struct sigaction));
  act.sa_handler = f;
  if (-1 == sigaction(sigNo, &act, NULL))
    ERR("sigaction");
}

void child_work(int i)
{
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGUSR2);
  sigprocmask(SIG_BLOCK, &mask, NULL);

  while (1)
  {
    srand(time(NULL) * getpid());
    int m = 100 + rand() % (100);

    struct timespec t = {0, m * 1000000};

    for (int i = 0; i < 30; i++)
    {
      nanosleep(&t, NULL);

      sigset_t pendingMask;
      sigpending(&pendingMask);

      if (sigismember(&pendingMask, SIGUSR2))
      {
        printf("Child with pid %d received SIGUSR2. Terminating.\n", getpid());
        exit(EXIT_SUCCESS);
      }

      if (kill(getppid(), SIGUSR1))
        ERR("kill");

      printf("*");
      fflush(stdout);
    }
  }
}

void create_children(int n)
{
  pid_t s;
  for (n--; n >= 0; n--)
  {
    if ((s = fork()) < 0)
      ERR("Fork:");
    if (!s)
    {
      child_work(n);
      exit(EXIT_SUCCESS);
    }
  }
}

void parent_work()
{
  int sigCount = 0;
  int childCount = 0;

  while (1)
  {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    sigprocmask(SIG_BLOCK, &mask, NULL);

    int signalReceived;
    sigwait(&mask, &signalReceived);

    if (signalReceived == SIGUSR1)
    {
      ++sigCount;
      printf("[PARENT] received %d SIGUSR1\n", sigCount);

      ++childCount;

      if (sigCount == 100)
      {
        printf("[PARENT] Sending SIGUSR2 to terminate all remaining children.\n");
        kill(0, SIGUSR2);
        break;
      }
    }
    else if (signalReceived == SIGUSR2)
    {
      printf("[PARENT] Received SIGUSR2. Terminating.\n");
      break;
    }
  }

  while (wait(NULL) > 0)
  {
    // Wait for all remaining children to terminate
  }
}

void usage(char *name)
{
  fprintf(stderr, "USAGE: %s 0<n\n", name);
  exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
  int n;
  if (argc < 2)
    usage(argv[0]);
  n = atoi(argv[1]);
  if (n <= 0)
    usage(argv[0]);

  create_children(n);
  parent_work();
  return EXIT_SUCCESS;
}
