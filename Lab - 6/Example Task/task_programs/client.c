#define _GNU_SOURCE
#include <errno.h>
#include <mqueue.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

typedef struct queuenames
{
    char name_s[20];
    char name_d[20];
    char name_m[20];
    pid_t name_c;
} queuenames;

struct sum_message
{
    int a;
    int b;
};

volatile sig_atomic_t timed_out = 0;

void sethandler(void (*f)(int, siginfo_t *, void *), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_sigaction = f;
    act.sa_flags = SA_SIGINFO;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void handler(int sig, siginfo_t *info, void *ucontext)
{
    mqd_t mq = *(mqd_t *)info->si_value.sival_ptr;
    char msg[20]; // Adjust size as needed
    if (mq_receive(mq, msg, 8192, NULL) == -1)
    {
        perror("mq_receive");
        exit(1);
    }
    printf("Received message: %s\n", msg);
}

void notification_function(int signo, siginfo_t *info, void *context)
{
    printf("Notification received\n");
    mqd_t mq = *(mqd_t *)(info->si_value.sival_ptr);
    char msg[20]; // Adjust size as needed
    if (mq_receive(mq, msg, 8192, NULL) == -1)
    {
        perror("mq_receive");
        exit(1);
    }

    struct sigevent sev;
    sev.sigev_notify = SIGEV_SIGNAL; // Change to SIGEV_SIGNAL
    sev.sigev_signo = SIGUSR1;       // Specify the signal to be sent
    sev.sigev_value.sival_ptr = &mq;
    if (mq_notify(mq, &sev) == -1)
    {
        perror("mq_notify nf client");
        exit(1);
    }
    printf("Received message: %s\n", msg);
}

void *write_to_queue(void *arg)
{
    queuenames q_names = *(queuenames *)arg;

    while (timed_out==0)
    {
        mqd_t mq_s;
        struct sum_message msg;
        printf("Enter two numbers (or 'q' to quit): ");
        fflush(stdout);

        if (scanf("%d %d", &msg.a, &msg.b) == 2)
        {
            char message[20];
            sprintf(message, "%d %d %d", msg.a, msg.b, q_names.name_c);
            printf("Where do u wanna send it?\n");
            printf("1. %s\n", q_names.name_s);
            printf("2. %s\n", q_names.name_d);
            printf("3. %s\n", q_names.name_m);
            int choice;
            scanf("%d", &choice);
            switch (choice)
            {
            case 1:
                mq_s = mq_open(q_names.name_s, O_RDWR);
                printf("Opened %s\n", q_names.name_s);
                break;
                
            case 2:
                mq_s = mq_open(q_names.name_d, O_RDWR);
                printf("Opened %s\n", q_names.name_d);
                break;
                
            case 3:
                mq_s = mq_open(q_names.name_m, O_RDWR);
                printf("Opened %s\n", q_names.name_m);
                break;
                
            }
            printf("Sending message: %s\n", message);

            if (mq_send(mq_s, message, strlen(message) + 1, 0) == -1)
            {
                perror("mq_send");
            }
        }
       
        mq_close(mq_s);
        
    }
    return NULL;

}
void *read_from_queue(void *arg)
{
    mqd_t mq = *(mqd_t *)arg;

    struct sigevent sev;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_notify_attributes = NULL;
    sev.sigev_signo = SIGUSR1;
    sev.sigev_value.sival_ptr = &mq;
    if (mq_notify(mq, &sev) == -1)
    {
        perror("mq_notify rq client");
    }

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 5; // 5 seconds timeout

    char msg[20]; // Adjust size as needed
    while (timed_out == 0)
    {
        if (mq_timedreceive(mq, msg, 8192, NULL, &ts) == -1)
        {
            if (errno == ETIMEDOUT)
            {
                printf("Receive timeout occurred\n");
                mq_close(mq);
                timed_out = 1;
                break;
            }
            perror("mq_receive");
        }
        else
        {
            printf("Received message: %s\n", msg);
        }
    }

    return NULL;
}

int main(int argc, char **argv)
{

    printf("I recived %s\n", argv[1]);

    queuenames args;

    sprintf(args.name_s, "/%s_s", argv[1]);
    sprintf(args.name_d, "/%s_d", argv[1]);
    sprintf(args.name_m, "/%s_m", argv[1]);
    printf("Queuesdfasas created %s %s %s\n", args.name_s, args.name_d, args.name_m);

    char pid_str[20];
    sprintf(pid_str, "/%d", getpid());
    args.name_c = getpid();

    sethandler(notification_function, SIGUSR1);

    mqd_t mq_mine = mq_open(pid_str, O_RDWR | O_CREAT, 0666, NULL);
    pthread_t write_thread, read_thread;
    pthread_create(&write_thread, NULL, write_to_queue, &args);

    pthread_create(&read_thread, NULL, read_from_queue, &mq_mine);

    while(timed_out==0)
    {
        ;
    }
    pthread_cancel(write_thread);
    pthread_cancel(read_thread);
    

    pthread_join(write_thread, NULL);
    pthread_join(read_thread, NULL);
    mq_close(mq_mine);
    mq_unlink(pid_str);
    // ClientWork(args);
    return 0;
}