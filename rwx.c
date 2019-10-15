#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <float.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>

/**
 * Compiling Guide
 * To show starvation, compile with 
 * gcc [this_filename].c -lpthread
 * To show solve of starvation, compile with
 * gcc [this_filename].c -lpthread -DEQUAL
 * To show values in variables, append
 * -DPNUM
 * to any of the above commands
 */
#define RNUMS 500
#define WNUMS 10
#define min(_a, _b) (_a < _b) ? (_a) : (_b)
#define max(_a, _b) (_a < _b) ? (_b) : (_a)
#define sleepr {                                                              \
    uint32_t usecs = (rand() % 100) * 1000;                                   \
    usleep(usecs);                                                            \
}

// Metrics
static long counter = 0;
static double time_count_read = 0.0;
static double time_min_read = DBL_MAX;
static double time_max_read = 0.0;
static long access_count_read = 0;
static double time_count_write = 0.0;
static double time_min_write = DBL_MAX;
static double time_max_write = 0.0;
static long access_count_write = 0;

// Synchronization Variables
#ifdef EQUAL
static sem_t queue_mutex;
#endif 
static sem_t rw_mutex, mutex;
static int read_count;

int isnumber(const char*s) {
    char* e = NULL;
    strtol(s, &e, 0);
    return e != NULL && *e == '\0';
}

static void* writer(void *niter) {
    int local_count = 0;
    do {
        clock_t begin = clock();
#ifdef EQUAL
        if (sem_wait(&queue_mutex) == -1)
            exit(2);
#endif 
        if (sem_wait(&rw_mutex) == -1)
            exit(2);
        // Critical section START
        // Update metrics
        clock_t end = clock();
        double wait_time = (double)((end - begin) * 1000 / CLOCKS_PER_SEC) * 1000;
        time_count_write += wait_time;
        time_max_write = max(time_max_write, wait_time);
        time_min_write = min(time_min_write, wait_time);
        access_count_write += 1;
        sleepr;
        // Write
        counter += 10;
        // Critical section END
        if (sem_post(&rw_mutex) == -1)
            exit(2);
#ifdef EQUAL
        if (sem_post(&queue_mutex) == -1)
            exit(2);
#endif 
    } while ((++local_count) < *(int *)niter);
    return NULL;
}

static void* reader(void *niter) {
    int local_count = 0;
    do {
        clock_t begin = clock();
#ifdef EQUAL
        if (sem_wait(&queue_mutex) == -1)
            exit(2);
#endif 
        if (sem_wait(&mutex) == -1)
            exit(2);
        read_count += 1;
        if (read_count == 1)
            if (sem_wait(&rw_mutex) == -1)
                exit(2);
        if (sem_post(&mutex) == -1)
            exit(2);
#ifdef EQUAL
        if (sem_post(&queue_mutex) == -1)
            exit(2);
#endif 
        // Critical section START
        // Update metrics
        clock_t end = clock();
        double wait_time = (double)((end - begin) * 1000 / CLOCKS_PER_SEC) * 1000;
        time_count_read += wait_time;
        time_max_read = max(time_max_read, wait_time);
        time_min_read = min(time_min_read, wait_time);
        access_count_read += 1;
        // Read
#ifdef PNUM
        printf("count at this read is %ld\n", counter);
#endif
        sleepr;
        // Critical section END
        if (sem_wait(&mutex) == -1)
            exit(2);
        read_count -= 1;
        if (read_count == 0)
            if (sem_post(&rw_mutex) == -1)
                exit(2); 
        if (sem_post(&mutex) == -1)
            exit(2);
    } while ((++local_count) < *(int *)niter);   
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 3 || !isnumber(argv[1])) {
        printf("please specify niter\n");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&mutex, 0, 1) == -1 ||
        sem_init(&rw_mutex, 0, 1) == -1
#ifdef EQUAL 
        || sem_init(&queue_mutex, 0, 1) == -1
#endif
       ) {
        printf("Error, init semaphore\n");
        exit(EXIT_FAILURE);
    }
    time_t t;
    srand((unsigned)time(&t));
    read_count = 0;
    int niter = atoi(argv[1]);
    int nitew = atoi(argv[2]);
    pthread_t readers[RNUMS], writers[WNUMS];
    memset(readers, 0, RNUMS * sizeof(pthread_t));
    memset(writers, 0, WNUMS * sizeof(pthread_t));
    for (int i = 0; i < RNUMS; i++) {
        if (pthread_create(&readers[i], NULL, reader, &niter)) {
            printf("error, create reader\n");
            exit(EXIT_FAILURE);
        }
    }
    for (int i = 0; i < WNUMS; i++) {
        if (pthread_create(&writers[i], NULL, writer, &nitew)) {
            printf("error, create writer\n");
            exit(EXIT_FAILURE);
        }
    }
    for (int i = 0; i < RNUMS; i++) {
        if (pthread_join(readers[i], NULL)) {
            printf("error, join reader\n");
            exit(EXIT_FAILURE);
        }
    }
    for (int i = 0; i < WNUMS; i++) {
        if (pthread_join(writers[i], NULL)) {
            printf("error, join writer\n");
            exit(EXIT_FAILURE);
        }
    }
    printf("=============[SUMMARY]============\n");
    printf("[READER]>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
    printf("[Max wait] %f ms\n", time_max_read);
    printf("[Min wait] %f ms\n", time_min_read);
    printf("[Avg wait] %f ms\n", (time_count_read / access_count_read));
    printf("[Writer]>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
    printf("[Max wait] %f ms\n", time_max_write);
    printf("[Min wait] %f ms\n", time_min_write);
    printf("[Avg wait] %f ms\n", (time_count_write / access_count_write));
    return EXIT_SUCCESS;
}
