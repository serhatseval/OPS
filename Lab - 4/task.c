#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef struct list_node
{
    char *line;
    struct list_node *next;
} list_node_t;

typedef struct thread_data
{
    pthread_t pid;
    off_t size;
    off_t chunk;
    list_node_t *head;
    char *fd;
    int n;
} thread_data_t;

int next_chunk = 0;
int error_line = 0; // Global variable to store the line number of the error
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void print_list(list_node_t *head)
{
    list_node_t *tmp = head;

    while (tmp != NULL)
    {
        printf("%s\n", tmp->line);
        tmp = tmp->next;
    }
}

void free_list(list_node_t *head)
{
    list_node_t *tmp;

    while (head != NULL)
    {
        tmp = head;
        head = head->next;
        free(tmp->line);
        free(tmp);
    }
}
char *my_strdup(const char *src) {
    char *str;
    char *p;
    int len = 0;

    while (src[len])
        len++;
    str = malloc(len + 1);
    p = str;
    while (*src)
        *p++ = *src++;
    *p = '\0';
    return str;
}

void *thread_work(void *arg)
{
    thread_data_t *data = (thread_data_t *)arg;
    char *buffer = malloc(data->chunk + 1);
    if (buffer == NULL)
    {
        perror("malloc");
        pthread_mutex_lock(&mutex);
        error_line = __LINE__; // Store the line number of the error
        pthread_mutex_unlock(&mutex);
        return NULL;
    }

    while (1)
    {
        pthread_mutex_lock(&mutex);
        if (next_chunk >= data->size / data->chunk)
        {
            pthread_mutex_unlock(&mutex);
            break;
        }
        off_t chunk_start = next_chunk * data->chunk;
        next_chunk++;
        pthread_mutex_unlock(&mutex);

        // Read the chunk into the buffer
        if (pread(data->fd, buffer, data->chunk, chunk_start) < 0)
        {
            perror("pread");
            pthread_mutex_lock(&mutex);
            error_line = __LINE__; // Store the line number of the error
            pthread_mutex_unlock(&mutex);
            free(buffer);
            return NULL;
        }
        buffer[data->chunk] = '\0';

        // Add the buffer to the linked list
        list_node_t *new_node = malloc(sizeof(list_node_t));
        if (new_node == NULL)
        {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        new_node->line = my_strdup(buffer);
        new_node->next = data->head;
        data->head = new_node;

        printf("Thread %ld: start = %ld, size = %ld\n", data->pid, chunk_start, data->chunk);
    }

    print_list(data->head);
    free(buffer);
    return NULL;
}

void* concat_and_print(void* arg) {
    thread_data_t* threads = (thread_data_t*)arg;
    list_node_t *head = NULL, *tail = NULL;

    for (int i = 0; i < threads->n; i++) {
        if (pthread_join(threads[i].pid, NULL) != 0) {
            perror("pthread_join");
            exit(EXIT_FAILURE);
        }
        if (threads[i].head != NULL) {
            if (head == NULL) {
                head = threads[i].head;
            } else {
                tail->next = threads[i].head;
            }
            list_node_t *tmp = threads[i].head;
            while (tmp->next != NULL) {
                tmp = tmp->next;
            }
            tail = tmp;
        }
    }

    print_list(head);

    free_list(head);

    return NULL;
}

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        printf("Usage: %s <n> <m> <file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int n = atoi(argv[1]);
    int m = atoi(argv[2]);
    char *path = argv[3];

    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }

    struct stat st;
    if (fstat(fd, &st) < 0)
    {
        perror("fstat");
        exit(EXIT_FAILURE);
    }

    thread_data_t *threads = malloc(n * sizeof(thread_data_t));
    if (threads == NULL)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < n; i++)
    {
        threads[i].size = st.st_size;
        threads[i].chunk = st.st_size / m;
        threads[i].fd = fd;
        threads[i].head = NULL;
        threads[i].n = n;

        if (pthread_create(&threads[i].pid, NULL, thread_work, &threads[i]) != 0)
        {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

        pthread_t print_thread;
    if (pthread_create(&print_thread, NULL, concat_and_print, threads) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    if (pthread_join(print_thread, NULL) != 0) {
        perror("pthread_join");
        exit(EXIT_FAILURE);
    }

    

     if (error_line != 0) {
        fprintf(stderr, "Error occurred at line: %d\n", error_line);
        exit(EXIT_FAILURE);
    }

    free(threads);
    close(fd);
    return EXIT_SUCCESS;
}