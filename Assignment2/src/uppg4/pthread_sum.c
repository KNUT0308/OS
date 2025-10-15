#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int num_threads = 0;
double *array;
void *thread_func(); /* the thread function */
pthread_mutex_t lock;
int *parallel_histogram = NULL;
int num_of_elem;

/* generate a random floating point number from min to max */
double randfrom(double min, double max) {
    double range = (max - min);
    double div = RAND_MAX / range;
    return min + (rand() / div);
}

void print_histogram(int histogram1[], int histogram2[], double time1, double time2) {
    for (int i = 0; i < 30; i++) {
        printf("Bin %d: %d \t \t \t %d\n", i + 1, histogram1[i], histogram2[i]);
    }
    printf("Time: %.3f ms \t \t \t %0.3f\n ms", time1, time2);

}
int main(int argc, char *argv[]) {
    num_threads = atoi(argv[1]);
    num_of_elem = atoi(argv[2]);
    int num_of_bins = 30;
    int *serial_histogram = (int *)calloc(num_of_bins, sizeof(int));
    parallel_histogram = (int *)calloc(num_of_bins, sizeof(int));

    /* Initialize an array of random values */
    array = calloc(num_of_elem, sizeof(double));
    for (int i = 0; i < num_of_elem; i++)
        array[i] = randfrom(0, 1);

    /* Initialize histogram bins to 0 */
    for (int i = 0; i < num_of_bins; i++) {
        serial_histogram[i] = 0;
        parallel_histogram[i] = 0;
    }

    /* Perform Serial Sum */
    double time_serial = 0.0;
    // Timer Begin
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < num_of_elem; i++) {
        serial_histogram[(int)(array[i] * num_of_bins)]++;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_serial = (end.tv_nsec - start.tv_nsec) / 1e6;

    // Timer End



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

    printf("SERIAL HISTOGRAM: \t \t PARALLEL HISTOGRAM\n");
    print_histogram(serial_histogram, parallel_histogram, time_serial, time_parallel);

    /*free up resources properly */
    pthread_mutex_destroy(&lock);
    free(array);
    free(workers);
    free(ids);
}

void *thread_func(void *arg) {
    /* Assign each thread an id so that they are unique in range [0, num_thread -1 ] */
    int my_id = *(int *)arg;

    int interval = num_of_elem / num_threads;
    int start = my_id * interval;
    int end = (my_id == num_threads - 1) ? num_of_elem : start + interval;
    /* Perform Partial Parallel Sum Here */
    int my_histogram[30];
    for (int i = 0; i < 30; i++)
    {
        my_histogram[i] = 0;
    }
    

    for (int i = start; i < end; i++) {
        my_histogram[(int)(array[i] * 30)]++;
    }

    pthread_mutex_lock(&lock);
    for (int i = 0; i < 30; i++) {
        parallel_histogram[i] += my_histogram[i];
    }
    pthread_mutex_unlock(&lock);
    pthread_exit(0);
}
