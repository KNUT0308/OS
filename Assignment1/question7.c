#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* generate a random floating point number from min to max */
double randfrom(double min, double max) 
{
    double range = (max - min); 
    double div = RAND_MAX / range;
    return min + (rand() / div);
}

double sum(double arr[], int a, int b){
    double result = 0;
    for (int i = a; i < b; i++){
        result = result + arr[i];
    }
    return result;
}

double questionSeven(int N){
    double arr[N];
    // double firstSum;
    // double lastSum;
    double totalSum = 0;
    struct timespec start, end;
    double duration;
    

    for (int i = 0; i < N; i++){
        arr[i] = randfrom(0, 1);
        //printf("index %d gives %f\n", i, arr[i]);
    }

    int fd[2];
    pipe(fd);

    clock_gettime(CLOCK_MONOTONIC, &start); // Get the current time

    pid_t pid1 = fork(); // Create the first child process
    if (pid1 == 0) {
        double firstSum = sum(arr, 0, N/2);
        write(fd[1], &firstSum, sizeof(firstSum));
        // printf("firstSum: %f\n", firstSum);
        exit(0);
    }

    pid_t pid2 = fork(); // Create the second child process
    if (pid2 == 0) {
        double lastSum = sum(arr, N/2, N);
        write(fd[1], &lastSum, sizeof(lastSum));
        // printf("lastSum: %f\n", lastSum);
        exit(0);
    }

    close(fd[1]); // parent closes write end
    
    waitpid(pid1, NULL, 0); //Wait for first child to finish
    waitpid(pid2, NULL, 0); //Wait for second child to finish

   
    
    double part; // The sum returned through a pipe from the child process

    for (int i = 0; i < 2; i++) {
        read(fd[0], &part, sizeof(part));
        totalSum += part;
    }

    clock_gettime(CLOCK_MONOTONIC, &end); //Get the current time after child processes finish

    duration = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("total sum = %f\n", totalSum);
    // printf("Elapsed time: %f seconds\n", duration);

    return duration;
}




int main() {

    int i = 0;
    int N = 100;
    double average_time = 0;

    printf("Enter value for N: ");
    scanf("%d", &N);

    while(i<100){
        average_time = average_time + questionSeven(N);
        i++;
    }
    printf("Average time for N = %d is %f\n", N, (double)(average_time / 100));
    return 0;
    
}
