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

#define RNUMS 500
#define WNUMS 10

#define min(_a, _b) (_a < _b) ? (_a) : (_b)
#define max(_a, _b) (_a < _b) ? (_b) : (_a)

#define sleepr usleep((uint32_t)((rand() % 100) * 1000))
#define wait(_sem) if (sem_wait(&_sem) == -1) exit(2);
#define post(_sem) if (sem_post(&_sem) == -1) exit(2);

#define update_metric(_wtime, _rw) {                                          \
    time_count_##_rw += _wtime;                                               \
    time_max_##_rw = max(time_max_##_rw, _wtime);                             \
    time_min_##_rw = min(time_min_##_rw, _wtime);                             \
    access_count_##_rw += 1;                                                  \
}

// metrics (ms)
static double time_count_read = 0.0;
static double time_min_read = DBL_MAX;
static double time_max_read = 0.0;
static long access_count_read = 0;
static double time_count_write = 0.0;
static double time_min_write = DBL_MAX;
static double time_max_write = 0.0;
static long access_count_write = 0;

// semaphore variables
#ifdef EQUAL
static sem_t queue_mutex;
#endif 
static sem_t rw_mutex, mutex;
static int read_count;

// atomic variable
static long counter = 0;

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
        wait(queue_mutex);
#endif 
        wait(rw_mutex);
        // update metrics
        clock_t end = clock();
        double wait_time = ((double)(end - begin) * 1000 / CLOCKS_PER_SEC); 
        update_metric(wait_time, write);
        sleepr;
        // write
        counter += 10;
        post(rw_mutex);        
#ifdef EQUAL
        post(queue_mutex);
#endif 
    } while ((++local_count) < *(int *)niter);
    return NULL;
}

static void* reader(void *niter) {
    int local_count = 0;
    do {
        clock_t begin = clock();
#ifdef EQUAL
        wait(queue_mutex);
#endif 
        wait(mutex);
        read_count += 1;
        if (read_count == 1)
            wait(rw_mutex);
        post(mutex);
#ifdef EQUAL
        post(queue_mutex);
#endif 
        // update metrics
        clock_t end = clock();
        double wait_time = ((double)(end - begin) * 1000 / CLOCKS_PER_SEC);
        sleepr;
        // read
#ifdef PNUM
        printf("count at this read is %ld\n", counter);
#endif
        wait(mutex);
        read_count -= 1;
        update_metric(wait_time, read);
        if (read_count == 0)
            post(rw_mutex);
        post(mutex);
    } while ((++local_count) < *(int *)niter);   
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 3 || !isnumber(argv[1]) || !isnumber(argv[2])) {
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
    int niter = atoi(argv[2]);
    int nitew = atoi(argv[1]);
    srand((unsigned)time(&t));
    read_count = 0;
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

#ifdef EQUAL 
    printf("==============[Fair]==============\n");
#else
    printf("========[Reader Preference]=======\n");
#endif 
    printf("[Reader]--------------------------\n");
    printf("[Max wait] %f ms\n", time_max_read);
    printf("[Min wait] %f ms\n", time_min_read);
    printf("[Avg wait] %f ms\n", (time_count_read / access_count_read));
    printf("[Count]    %ld  \n", access_count_read);
    printf("[Writer]--------------------------\n");
    printf("[Max wait] %f ms\n", time_max_write);
    printf("[Min wait] %f ms\n", time_min_write);
    printf("[Avg wait] %f ms\n", (time_count_write / access_count_write));
    printf("[Count]    %ld  \n", access_count_write);
    return EXIT_SUCCESS;
}
