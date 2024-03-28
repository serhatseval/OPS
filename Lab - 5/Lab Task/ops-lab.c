#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define MIN_STUDENTS 4
#define MAX_STUDENTS 20

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(expression)             \
    (__extension__({                               \
        long int __result;                         \
        do                                         \
            __result = (long int)(expression);     \
        while (__result == -1L && errno == EINTR); \
        __result;                                  \
    }))
#endif
int done_students = 0;
volatile sig_atomic_t last_signal = 0;
int set_handler(void (*f)(int), int sig)
{
    struct sigaction act = {0};
    act.sa_handler = f;
    if (sigaction(sig, &act, NULL) == -1)
        return -1;
    return 0;
}

void sig_handler(int sig) { last_signal = sig; }

void student_work(int writing_pipe, int reading_pipe)
{
    pid_t pid = getpid();
    srand(pid);

    int k = rand() % 7 + 3;

    char buffer[100];

    while (TEMP_FAILURE_RETRY(read(reading_pipe, buffer, sizeof(buffer))) != sizeof(buffer))
    {
        // WAIT
    }

    printf("Student [%d]: HERE\n", pid);

    sprintf(buffer, "Student [%d]: HERE", pid);
    if (write(writing_pipe, buffer, sizeof(buffer)) != sizeof(buffer))
    {
        ERR("write");
    }
    int i = 0;
    int stage;
    while (i < 4)
    {
        int t = rand() % 400 + 100;
        usleep(t * 1000);
        int q = rand() % 20 + 1;
        int result = k + q;
        stage = i + 1;
        sprintf(buffer, "%d, %d, %d", result, pid, stage);
        if (write(writing_pipe, buffer, sizeof(buffer)) != sizeof(buffer))
        {
            ERR("write");
        }
        while (TEMP_FAILURE_RETRY(read(reading_pipe, buffer, sizeof(buffer))) != sizeof(buffer))
        {
            // WAIT
        }
        if (buffer[0] == '1')
        {
            printf("Student [%d]: I NAILED IT\n", pid);
            i++;
        }
        else if (strcmp(buffer, "done"))
        {
            printf("Student [%d]: OH NO I NEED MORE TIME TO FIX STAGE >:( %d", pid, stage);
            break;
        }
        else
        {
        }
    }
    sprintf(buffer, "%d, %d", pid, stage);
    if (write(writing_pipe, buffer, sizeof(buffer)) != sizeof(buffer))
    {
        ERR("write");
    }
    printf("Student exiting\n");
    exit(EXIT_SUCCESS);
}
void teacher_work(int n, int writing_pipe[n][2], int reading_pipe, int* pids)
{
    pid_t mypid = getpid();
    srand(mypid);
    char buffer[100];
    for (int i = 0; i < n; i++)
    {
        sprintf(buffer, "Teacher: Is [%d]: HERE", pids[i]);
        if (write(writing_pipe[i][1], buffer, sizeof(buffer)) != sizeof(buffer))
        {
            ERR("write");
        }
        printf("Teacher: Is [%d]: HERE\n", pids[i]);

        while (TEMP_FAILURE_RETRY(read(reading_pipe, buffer, sizeof(buffer))) != sizeof(buffer))
        {
            // WAIT
        }
    }
    alarm(2);
    while (last_signal != SIGALRM && done_students != 4)
    {
        while (TEMP_FAILURE_RETRY(read(reading_pipe, buffer, sizeof(buffer))) != sizeof(buffer))
        {
            // WAIT
        }
        int result, pid, stage;
        if (sscanf(buffer, "%d, %d, %d", &result, &pid, &stage) != 3)
        {
            ERR("sscanf");
        }
        int studentnumber;
        for (studentnumber = 0; studentnumber < n; studentnumber++)
        {
            if (pids[studentnumber] == pid)
            {
                break;
            }
        }
        int random = rand() % 20 + 1;
        int difficulty;
        switch (stage)
        {
            case 1:
                difficulty = random + 3;
                break;
            case 2:
                difficulty = random + 6;
                break;
            case 3:
                difficulty = random + 7;
                break;
            case 4:
                difficulty = random + 5;
                break;
        }
        if (result >= difficulty)
        {
            sprintf(buffer, "1");
            printf("Teacher: Student [%d] finished stage %d\n", pid, stage);
            if (write(writing_pipe[studentnumber][1], buffer, sizeof(buffer)) != sizeof(buffer))
            {
                ERR("write");
            }
            if (stage == 4)
            {
                done_students++;
            }
        }
        else
        {
        }
        if (done_students == n)
        {
            break;
        }
    }
    printf("Teacher: END OF TIME HAHAHAHAHAHAHHA \n");

    for (int i = 0; i < n; i++)
    {
        sprintf(buffer, "done");
        if (write(writing_pipe[i][1], buffer, sizeof(buffer)) != sizeof(buffer))
        {
            ERR("write");
        }
        while (TEMP_FAILURE_RETRY(read(reading_pipe, buffer, sizeof(buffer))) != sizeof(buffer))
        {
            // WAIT
        }
        int finalpid, finalstage, score;
        if (sscanf(buffer, "%d, %d", &finalpid, &finalstage) != 2)
        {
            ERR("sscanf");
        }
        switch (finalstage)
        {
            case 1:
                score = 3;
                break;
            case 2:
                score = 9;
                break;
            case 3:
                score = 16;
                break;
            case 4:
                score = 21;
                break;
        }
        printf("Teacher: %d - %d\n", finalpid, score);
    }
    printf("Teacher: IT'S FINALLY OVER\n");
}

void create_students(int n, int pipes_for_students[n][2], int writing_to_teacher_pipe[2], int* pids)
{
    for (int i = 0; i < n; i++)
    {
        if (pipe(pipes_for_students[i]) == -1)
        {
            ERR("pipe");
        }
    }

    for (int i = 0; i < n; ++i)
    {
        int temp_pid;
        switch (temp_pid = fork())
        {
            case 0:
            {
                for (int j = 0; j < n; j++)
                {
                    if (j != i)
                    {
                        close(pipes_for_students[j][0]);
                    }
                    close(pipes_for_students[j][1]);
                }
                close(writing_to_teacher_pipe[0]);
                student_work(writing_to_teacher_pipe[1], pipes_for_students[i][0]);

                break;
            }
            case -1:
            {
                ERR("fork");
            }
        }
        pids[i] = temp_pid;
    }
    for (int i = 0; i < n; i++)
    {
        close(pipes_for_students[i][0]);
    }
    close(writing_to_teacher_pipe[1]);
}

void usage(char* name)
{
    fprintf(stderr, "USAGE: %s N M\n", name);
    fprintf(stderr, "N: 4 <= N <= 20 - number of students\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv)
{
    int n;
    set_handler(sig_handler, SIGALRM);
    if (argc != 2)
        usage(argv[0]);
    n = atoi(argv[1]);
    if (n < MIN_STUDENTS || n > MAX_STUDENTS)
        usage(argv[0]);
    int* pids;
    pids = (int*)malloc(n * sizeof(int));
    int writing_to_teacher_pipe[2];
    int pipes_for_students[n][2];
    pipe(writing_to_teacher_pipe);

    create_students(n, pipes_for_students, writing_to_teacher_pipe, pids);

    teacher_work(n, pipes_for_students, writing_to_teacher_pipe[0], pids);
    while (wait(NULL) > 0)
        ;

    return EXIT_SUCCESS;
}
