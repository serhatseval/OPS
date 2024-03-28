#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <mqueue.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define MAX_QUEUE_NAME 256
#define TASK_QUEUE_NAME "/task_queue_%d"
#define RESULT_QUEUE_NAME "/result_queue_%d_%d"

#define MAX_MSG_SIZE 8  // Max message size
#define MAX_MSGS 10     // Queue size
#define MAX_NAME_LEN 50

#define MIN_WORKERS 2
#define MAX_WORKERS 20
#define MIN_TIME 100
#define MAX_TIME 5000
#define TASK_TODO 5

volatile sig_atomic_t children_left = 0;

int pids[MAX_WORKERS];
mqd_t wq[MAX_WORKERS];

struct task
{
    float a;
    float b;
};

int sethandler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        return -1;
    return 0;
}

void setnotifyhandler(void (*f)(int, siginfo_t *, void *), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_sigaction = f;
    act.sa_flags = SA_SIGINFO;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void mq_handler(int sig, siginfo_t *info, void *p)
{
    sig = sig;
    p = p;
    float res;
    unsigned msg_prio;

    static struct sigevent not ;
    not .sigev_notify = SIGEV_SIGNAL;
    not .sigev_signo = SIGRTMIN;
    not .sigev_value.sival_ptr = (mqd_t *)info->si_value.sival_ptr;
    if (mq_notify(*(mqd_t *)info->si_value.sival_ptr, &not ) < 0)
        ERR("mq_notify");
    ;
    for (int i = 0; i < MAX_WORKERS; i++)
    {
        if (pids[i] == 0)
            break;

        mqd_t q = wq[i];
        for (;;)
        {
            if (mq_receive(q, (char *)&res, sizeof(float), &msg_prio) < 1)
            {
                if (errno == EAGAIN)
                    break;
                else
                    ERR("mq_receive");
            }
            printf("Result from worker %d : %f\n", pids[i], res);
        }
    }
}
/*
void mq_handler(int sig, siginfo_t *info, void *p)
{
    mqd_t *q;
    float res;
    unsigned msg_prio;

    q = (mqd_t *)info->si_value.sival_ptr;

    static struct sigevent not ;
    not .sigev_notify = SIGEV_SIGNAL;
    not .sigev_signo = SIGRTMIN;
    not .sigev_value.sival_ptr = q;
    if (mq_notify(*q, &not ) < 0)
        ERR("mq_notify");

    for (;;)
    {
        if (mq_receive(*q, (char *)&res, sizeof(float), &msg_prio) < 1)
        {
            if (errno == EAGAIN)
                break;
            else
                ERR("mq_receive");
        }
        int i;
        for(i = 0; i < MAX_WORKERS; i++)
        {
            if(pids[i] == 0)
                ERR("Bad notify");

            if(wq[i] == *q)
                break;
        }
        printf("Result from worker %d : %f\n", pids[i], res);
    }
}
*/
void child_work(mqd_t tq, int spid)
{
    long millis;
    struct timespec sleept;
    srand(getpid());

    mqd_t q;
    struct mq_attr attr;
    attr.mq_maxmsg = MAX_MSGS;
    attr.mq_msgsize = sizeof(float);

    char buf[MAX_NAME_LEN];
    snprintf(buf, MAX_NAME_LEN, "/result_queue_%d_%d", spid, getpid());
    if ((q = TEMP_FAILURE_RETRY(mq_open(buf, O_RDWR | O_CREAT, 0600, &attr))) == (mqd_t)-1)
        ERR("mq open task queue");

    printf("[%d] Worker ready!\n", getpid());
    for (int i = 0; i < 5; i++)
    {
        struct task t;

        if (mq_receive(tq, (char *)&t, (size_t)sizeof(t), NULL) < 8)
            ERR("mq receive");

        printf("[%d] Received task [%f, %f]\n", getpid(), t.a, t.b);

        millis = 500 + rand() % 1500;
        sleept.tv_sec = millis / 1000;
        sleept.tv_nsec = (millis % 1000) * 1000 * 1000;

        nanosleep(&sleept, NULL);

        float res = t.a + t.b;

        if (TEMP_FAILURE_RETRY(mq_send(q, (const char *)&res, sizeof(float), 0)))
            ERR("mq_send");

        printf("[%d] Result sent [%f]\n", getpid(), res);
    }

    printf("[%d] Exits!\n", getpid());
}

void parent_work(int n, mqd_t tq, int T1, int T2)
{
    long millis;
    struct timespec sleept;
    struct timespec sendt;
    sendt.tv_sec = 0;
    sendt.tv_nsec = 500;
    srand(getpid());

    struct mq_attr attr;
    attr.mq_maxmsg = MAX_MSGS;
    attr.mq_msgsize = sizeof(float);

    for (int i = 0; i < n; i++)
    {
        char buf[MAX_NAME_LEN];
        snprintf(buf, MAX_NAME_LEN, "/result_queue_%d_%d", getpid(), pids[i]);
        if ((wq[i] = TEMP_FAILURE_RETRY(mq_open(buf, O_RDWR | O_CREAT | O_NONBLOCK, 0600, &attr))) == (mqd_t)-1)
            ERR("mq open task queue");

        static struct sigevent not ;
        not .sigev_notify = SIGEV_SIGNAL;
        not .sigev_signo = SIGRTMIN;
        not .sigev_value.sival_ptr = &wq[i];
        if (mq_notify(wq[i], &not ) < 0)
            ERR("mq_notify");
    }
    printf("Server is starting...\n");

    for (int i = 0; i < 5 * n; i++)
    {
        struct task t;

        t.a = ((float)(rand() % 1000)) / 10;
        t.b = ((float)(rand() % 1000)) / 10;

        if (mq_timedsend(tq, (char *)&t, (size_t)sizeof(t), 1, &sendt) == 0)
        {
            printf("New task queued: [%f, %f]\n", t.a, t.b);
        }
        else
        {
            i--;
            if (errno == EAGAIN)
                printf("Queue is full!");
        }

        millis = T1 + rand() % (T2 - T1);
        sleept.tv_sec = millis / 1000;
        sleept.tv_nsec = (millis % 1000) * 1000 * 1000;
        nanosleep(&sleept, NULL);
    }
}

void create_children(int n, mqd_t tq)
{
    int pid, spid = getpid();
    while (n-- > 0)
    {
        switch (pid = fork())
        {
            case 0:
                child_work(tq, spid);
                if (mq_close(tq) < 0)
                    ERR("mq close");
                exit(EXIT_SUCCESS);
            case -1:
                perror("Fork:");
                exit(EXIT_FAILURE);
        }
        pids[children_left] = pid;
        children_left++;
    }
}

void usage(const char *name)
{
    fprintf(stderr, "USAGE: %s N T1 T2\n", name);
    fprintf(stderr, "N: %d <= N <= %d - number of workers\n", MIN_WORKERS, MAX_WORKERS);
    fprintf(stderr, "T1, T2: %d <= T1 < T2 <= %d - time range for spawning new tasks\n", MIN_TIME, MAX_TIME);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    int n, T1, T2;
    if (argc != 4)
        usage(argv[0]);
    n = atoi(argv[1]);
    if (n < MIN_WORKERS || n > MAX_WORKERS)
        usage(argv[0]);

    T1 = atoi(argv[2]);
    T2 = atoi(argv[3]);
    if (T1 < MIN_TIME || T2 < T1 || T2 > MAX_TIME)
        usage(argv[0]);

    mqd_t tq;
    struct mq_attr attr;
    attr.mq_maxmsg = MAX_MSGS;
    attr.mq_msgsize = MAX_MSG_SIZE;
    char buf[MAX_NAME_LEN];
    snprintf(buf, MAX_NAME_LEN, "/task_queue_%d", getpid());
    if (mq_unlink(buf) == -1 && errno != ENOENT)
        ERR("mq unlink");
    if ((tq = TEMP_FAILURE_RETRY(mq_open(buf, O_RDWR | O_CREAT, 0600, &attr))) == (mqd_t)-1)
        ERR("mq open task queue");

    sethandler(SIG_IGN, SIGCHLD);
    setnotifyhandler(mq_handler, SIGRTMIN);

    create_children(n, tq);

    
    parent_work(n, tq, T1, T2);

    printf("Before wait.\n");
    int ret;//, stat;

    //while((ret = sleep(5)));

    
    // for (int i = 0; i < n; i++) {
    //     errno = 0;
    //     ret = waitpid(pids[i], &stat, 0);
    //     printf("ret = %d\n", ret);
    //     perror("wait()");
    //     if (ret == -1 && errno == EINTR) i--;
    // }
    // while(n--)
    //     if(TEMP_FAILURE_RETRY(wait(NULL)))
    //         ERR("wait");
    

    while ((ret = TEMP_FAILURE_RETRY(wait(NULL))) > 0)
        TEMP_FAILURE_RETRY(printf("Waited %d!\n", ret));

    printf("All workers have finished.\n");

    mq_close(tq);
    if (mq_unlink(buf))
        ERR("mq unlink");

    sleep(5);

    return EXIT_SUCCESS;
}
