#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int num_threads = 0;
float *array;
void *thread_func(); /* the thread function */
pthread_mutex_t lock;
float sum_parallel = 0.0f; // Change from double to float

/* generate a random floating point number from min to max */
float randfrom(float min, float max) {
    float range = (max - min);
    float div = RAND_MAX / range;
    return min + (rand() / div);
}
int main(int argc, char *argv[]) {
    num_threads = atoi(argv[1]);

    /* Initialize an array of random values */
    array = malloc(1000000 * sizeof(float));
    for (int i = 0; i < 1000000; i++)
        array[i] = randfrom(0, 1);

    /* Perform Serial Sum */
    float sum_serial = 0.0;
    double time_serial = 0.0;
    // Timer Begin
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < 1000000; i++) {
        sum_serial += array[i];
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_serial = (end.tv_nsec - start.tv_nsec) / 1e6;

    // Timer End
    printf("Serial Sum = %f, time = %.3f \n", sum_serial, time_serial);

    /* Create a pool of num_threads workers and keep them in workers */
    pthread_mutex_init(&lock, NULL);
    pthread_t *workers = malloc(num_threads * sizeof(pthread_t));
    int *ids = malloc(num_threads * sizeof(int));
    double time_parallel = 0.0;

    // Timer Begin
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < num_threads; i++) {
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        ids[i] = i;
        pthread_create(&workers[i], &attr, thread_func, &ids[i]);
    }

    for (int i = 0; i < num_threads; i++)
        pthread_join(workers[i], NULL);

    // Timer End

    clock_gettime(CLOCK_MONOTONIC, &end);
    time_parallel += (end.tv_nsec - start.tv_nsec) / 1e6;

    printf("Parallel Sum = %f , time = %.3f \n", sum_parallel, time_parallel);

    /*free up resources properly */
    pthread_mutex_destroy(&lock);
    free(array);
    free(workers);
    free(ids);
}

void *thread_func(void *arg) {
    /* Assign each thread an id so that they are unique in range [0, num_thread -1 ] */
    int my_id = *(int *)arg;

    int N = 1000000;
    int interval = N / num_threads;
    int start = my_id * interval;
    int end = (my_id == num_threads - 1) ? N : start + interval;
    /* Perform Partial Parallel Sum Here */
    float my_sum = 0.0f;

    for (int i = start; i < end; i++) {
        my_sum += array[i];
    }
    printf("Thread %d last sum: %f\n", my_id + 1, my_sum);

    pthread_mutex_lock(&lock);
    sum_parallel += my_sum; // No conversion needed
    pthread_mutex_unlock(&lock);
    pthread_exit(0);
}
