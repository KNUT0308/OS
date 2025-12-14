#include <pthread.h> 
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdatomic.h>

unsigned int bytesCount;
pthread_mutex_t byteCounterMutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
     off_t offset;
     size_t bytes;
} request; // Nickname "request"

typedef struct {
     request *requests;
     int numRequests;
     char* source; // What to write
     int destination; // Where to write, using a file descriptor
} writer_args; // Nickname "writer_args"

typedef struct {
     request *requests;
     int numRequests;
     char* result; // Store what has been read
     int source; // What to read from, using a file descriptor
} reader_args; // Nickname "reader_args"

void *reader_thread_func(void *arg) { 
     // @Add code for reader threads
     // @Given a list of [offset1, bytes1], [offset2, bytes2], ...
     // @for each: read bytes_i from offset_i
     reader_args *input = (reader_args *)arg;
     request* requests = input->requests;
     int numRequests = input->numRequests;
     char* result = input->result;
     int fileDescriptor = input->source;

     for (int i = 0; i < numRequests; i++) {
          off_t offset = requests[i].offset; // Offset of reading position for the specific request
          size_t bytes = requests[i].bytes; // Number of bytes to read for the specific request
          int readTot = 0; // Total amount that has been read
          char *destination = result + offset;

          while(readTot < bytes) { // Loop until all bytes read
               int readCur = pread(fileDescriptor, destination + readTot, bytes - readTot, offset + readTot); // Amount that was read for the current request
               if (readCur > 0) {
                    readTot += readCur; // There are more bytes to read and there was no error, continue
               } else if (readCur == 0) {
                    break; // No bytes were read, there's nothing more to read, we're done
               } else {
                    printf("ERROR: read for thread failed");
                    break;
               }               
          }
          pthread_mutex_lock(&byteCounterMutex);
          bytesCount += readTot;
          pthread_mutex_unlock(&byteCounterMutex);
     }     
     pthread_exit(0);
}

void *writer_thread_func(void *arg) { 
     // @Add code for writer threads
     // @Given a list of [offset1, bytes1], [offset2, bytes2], ...
     // @for each: write bytes_i to offset_i
     writer_args *input = (writer_args *)arg;
     request* requests = input->requests;
     int numRequests = input->numRequests;
     char* buffer = input->source;
     int fileDescriptor = input->destination;

     ssize_t write = 0; // Amount that was written for the current request
     for (int i = 0; i < numRequests; i++) {
          off_t offset = requests[i].offset; // Offset of writing position for the specific request
          size_t bytes = requests[i].bytes; // Number of bytes to write for the specific request
          int writeTot = 0; // Total amount that has been written
          char *source = buffer + offset;

          while(writeTot < bytes) { // Loop until all bytes written
               write = pwrite(fileDescriptor, source + writeTot, bytes - writeTot, offset + writeTot);
               if (write > 0) {
                    writeTot += write; // There are more bytes to write and there was no error, continue
               } else if (write == 0) {
                    break; // No bytes were written, there's nothing more to write, we're done
               } else {
                    printf("ERROR: write for thread failed");
                    break;
               }  
          }
          pthread_mutex_lock(&byteCounterMutex);
          bytesCount += writeTot;
          pthread_mutex_unlock(&byteCounterMutex);
     }     
     pthread_exit(0);
}


int main(int argc, char *argv[])
{
     int n = atoi(argv[1]); // Number of bytes
     int p = atoi(argv[2]); // Number of threads

     // @create a file for saving the data
     int fileDescriptor = open("fileSeq.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
     if (fileDescriptor < 0) {printf("ERROR: fileSeq.txt not opened for writing correctly"); return 1;}

     // @allocate a buffer and initialize it
     char* buffer = malloc(n); // Dynamically allocate n bytes in memory of type char, to be used as temporary storage space
     
     FILE* independence;
     independence = fopen("congress-07-04-1776.txt", "r"); // Open the Declaration of Independence in read-mode
     if (independence == NULL) { printf("ERROR: congress-07-04-1776.txt not handled correctly"); return 1;}
     
     for (int i = 0; i < n; i++) {
          int character = fgetc(independence);
          if (character != EOF) {
               buffer[i] = (char)character; // Write n bytes (characters) of the Declaration of Independence to the buffer
          } else {
               buffer[i] = 'x'; // If End-Of-File, write 'x' instead
          }
     }

     fclose(independence);
     

     // ***************** @create two lists of 100 requests in the format of [offset, bytes] *********************

     
     // @List 1: sequtial requests of 16384 bytes, where offset_n = offset_(n-1) + 16384
     // @e.g., [0, 16384], [16384, 16384], [32768, 16384] ...
     // @ensure no overlapping among these requests.

     request sequential[100];
     int numSeq = 0;
     off_t offset = 0;
     for (int i = 0; i < 100 && offset + 16384 <= n; i++) { // Create at most 100 requests, while also not creating an offset that surpasses n
          sequential[i].offset = offset;
          sequential[i].bytes = (size_t)16384;
          offset += 16384;
          numSeq++;
     }

     // @List 2: random requests of 128 bytes, where offset_n = random[0,N/4096] * 4096
     // @e.g., [4096, 128], [16384, 128], [32768, 128], etc.
     // @ensure no overlapping among these requests.
     
     // Approach: shuffle randomly to ensure no overlap
     int pages = n / 4096; // Divide up into 4096 pages
     int *index = malloc(pages * sizeof(int));
     for (int i = 0; i < pages; i++) {
          index[i] = i; // Initialize indexes with incrementing numbers
     }

     for (int i = pages-1; i > 0; i--) {
          int j = rand() % (i+1); // Randomly select an index smaller than the current index i
          int tmp = index[i]; // Swap the current index i with index j
          index[i] = index[j];
          index[j] = tmp;
     }

     request random[100];
     int numRand = 0;
     for (int i = 0; i < 100 && i < (int)pages; i++) {
          random[i].offset = (off_t)(index[i] * 4096);
          random[i].bytes = (size_t)128;
          numRand++;
     }

     free(index);


     // ****************** Write/read with List 1 (sequential) ******************

     /* Abbreviation explanations:
     - SW = Sequential write
     - SR = Sequential read
     - RW = Random write
     - RR = Random read
     */

     // @start timing 
     bytesCount = 0; // Ensure that count of written bytes begins at 0
     struct timespec start_SW, end_SW;
     clock_gettime(CLOCK_MONOTONIC, &start_SW);

     /* Create writer workers and pass in their portion of list1 */   
     pthread_t *workers_SW = malloc(p * sizeof(pthread_t)); // Space for thread IDs
     writer_args *args_SW = malloc(p * sizeof(writer_args)); // Space for thread arguments
     int handled = 0; // Number of requests handled
     int piece = numSeq/p; // Optimization proposal: Give the first threads a bit more to do since they start earlier, instead of giving the last thread more than the others
     for (int i = 0; i < p; i++) {
          if (i == p-1 && handled < numSeq) { 
               piece = numSeq-handled; // Case when last thread and there is more left, then give the last thread all the remaining requests
          }
          args_SW[i].requests = &sequential[handled];
          args_SW[i].numRequests = piece;
          args_SW[i].source = buffer;
          args_SW[i].destination = fileDescriptor;
          pthread_create(&workers_SW[i], NULL, writer_thread_func, &args_SW[i]);
          handled += piece;          
     }
     
     /* Wait for all writers to finish */ 
     for (int i = 0; i < p; i++) {
          pthread_join(workers_SW[i], NULL); // Make sure that each thread terminates before continuing
     }
     
     free(workers_SW);
     free(args_SW);

     // @close the file 
     close(fileDescriptor);

     // @end timing 
     clock_gettime(CLOCK_MONOTONIC, &end_SW);
     double time_SW = end_SW.tv_sec - start_SW.tv_sec + ((end_SW.tv_nsec - start_SW.tv_nsec) / 1e9);
     if (time_SW <= 0) {
          printf("ERROR: Zero or negative time");
          return 1;
     }

     //@Print out the write bandwidth
     double mb = (double)bytesCount/1000000;
     printf("Write %f MB, use %d threads, elapsed time %f s, write bandwidth: %f MB/s \n", mb, p, time_SW, mb/time_SW);
     
     // @reopen the file 
     fileDescriptor = open("fileSeq.txt", O_RDONLY);
     if (fileDescriptor < 0) {printf("ERROR: fileSeq.txt not opened for reading correctly"); return 1;}

     // @start timing 
     bytesCount = 0; // Ensure that count of read bytes begins at 0
     struct timespec start_SR, end_SR;
     clock_gettime(CLOCK_MONOTONIC, &start_SR);

     /* Create reader workers and pass in their portion of list1 */   
     pthread_t *workers_SR = malloc(p * sizeof(pthread_t)); // Space for thread IDs
     reader_args *args_SR = malloc(p * sizeof(reader_args)); // Space for thread arguments
     char* result_SR = malloc(n); // Place to store what all the threads have read
     handled = 0; // Number of requests handled
     piece = numSeq/p; // Optimization proposal: Give the first threads a bit more to do since they start earlier, instead of giving the last thread more than the others
     for (int i = 0; i < p; i++) {
          if (i == p-1 && handled < numSeq) { 
               piece = numSeq-handled; // Case when last thread and there is more left, then give the last thread all the remaining requests
          }
          args_SR[i].requests = &sequential[handled];
          args_SR[i].numRequests = piece;
          args_SR[i].result = result_SR;
          args_SR[i].source = fileDescriptor;
          pthread_create(&workers_SR[i], NULL, reader_thread_func, &args_SR[i]);
          handled += piece;          
     }
     
     /* Wait for all reader to finish */ 
     for (int i = 0; i < p; i++) {
          pthread_join(workers_SR[i], NULL); // Make sure that each thread terminates before continuing
     }

     free(workers_SR);
     free(args_SR);
     free(result_SR);

     // @close the file 
     close(fileDescriptor);

     // @end timing 
     clock_gettime(CLOCK_MONOTONIC, &end_SR);
     double time_SR = end_SR.tv_sec - start_SR.tv_sec + ((end_SR.tv_nsec - start_SR.tv_nsec) / 1e9);
     if (time_SR <= 0) {
          printf("ERROR: Zero or negative time");
          return 1;
     }

     //@Print out the read bandwidth
     mb = (double)bytesCount/1000000;
     printf("Read %f MB, use %d threads, elapsed time %f s, read bandwidth: %f MB/s \n", mb, p, time_SR, mb/time_SR);



     // ****************** Write/read with List 2 (random) ******************


     // @Repeat the write and read test now using List2 

     // @create a file for saving the data
     fileDescriptor = open("fileRand.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
     if (fileDescriptor < 0) {printf("ERROR: fileRand.txt not opened for writing correctly"); return 1;}

     // @start timing 
     bytesCount = 0; // Ensure that count of written bytes begins at 0
     struct timespec start_RW, end_RW;
     clock_gettime(CLOCK_MONOTONIC, &start_RW);

     /* Create writer workers and pass in their portion of list1 */   
     pthread_t *workers_RW = malloc(p * sizeof(pthread_t)); // Space for thread IDs
     writer_args *args_RW = malloc(p * sizeof(writer_args)); // Space for thread arguments
     handled = 0; // Number of requests handled
     piece = numRand/p; // Optimization proposal: Give the first threads a bit more to do since they start earlier, instead of giving the last thread more than the others
     for (int i = 0; i < p; i++) {
          if (i == p-1 && handled < numRand) { 
               piece = numRand-handled; // Case when last thread and there is more left, then give the last thread all the remaining requests
          }
          args_RW[i].requests = &random[handled];
          args_RW[i].numRequests = piece;
          args_RW[i].source = buffer;
          args_RW[i].destination = fileDescriptor;
          pthread_create(&workers_RW[i], NULL, writer_thread_func, &args_RW[i]);
          handled += piece;          
     }
     
     /* Wait for all writers to finish */ 
     for (int i = 0; i < p; i++) {
          pthread_join(workers_RW[i], NULL); // Make sure that each thread terminates before continuing
     }
     
     free(workers_RW);
     free(args_RW);

     // @close the file 
     close(fileDescriptor);

     // @end timing 
     clock_gettime(CLOCK_MONOTONIC, &end_RW);
     double time_RW = end_RW.tv_sec - start_RW.tv_sec + ((end_RW.tv_nsec - start_RW.tv_nsec) / 1e9);
     if (time_RW <= 0) {
          printf("ERROR: Zero or negative time");
          return 1;
     }

     //@Print out the write bandwidth
     mb = (double)bytesCount/1000000;
     printf("Write %f MB, use %d threads, elapsed time %f s, write bandwidth: %f MB/s \n", mb, p, time_RW, mb/time_RW);
     
     // @reopen the file 
     fileDescriptor = open("fileRand.txt", O_RDONLY);
     if (fileDescriptor < 0) {printf("ERROR: fileRand.txt not opened for reading correctly"); return 1;}

     // @start timing
     bytesCount = 0; // Ensure that count of read bytes begins at 0 
     struct timespec start_RR, end_RR;
     clock_gettime(CLOCK_MONOTONIC, &start_RR);

     /* Create reader workers and pass in their portion of list1 */   
     pthread_t *workers_RR = malloc(p * sizeof(pthread_t)); // Space for thread IDs
     reader_args *args_RR = malloc(p * sizeof(reader_args)); // Space for thread arguments
     char* result_RR = malloc(n); // Place to store what all the threads have read
     handled = 0; // Number of requests handled
     piece = numRand/p; // Optimization proposal: Give the first threads a bit more to do since they start earlier, instead of giving the last thread more than the others
     for (int i = 0; i < p; i++) {
          if (i == p-1 && handled < numRand) { 
               piece = numRand-handled; // Case when last thread and there is more left, then give the last thread all the remaining requests
          }
          args_RR[i].requests = &random[handled];
          args_RR[i].numRequests = piece;
          args_RR[i].result = result_RR;
          args_RR[i].source = fileDescriptor;
          pthread_create(&workers_RR[i], NULL, reader_thread_func, &args_RR[i]);
          handled += piece;          
     }
     
     /* Wait for all reader to finish */ 
     for (int i = 0; i < p; i++) {
          pthread_join(workers_RR[i], NULL); // Make sure that each thread terminates before continuing
     }

     free(workers_RR);
     free(args_RR);
     free(result_RR);

     // @close the file 
     close(fileDescriptor);

     // @end timing 
     clock_gettime(CLOCK_MONOTONIC, &end_RR);
     double time_RR = end_RR.tv_sec - start_RR.tv_sec + ((end_RR.tv_nsec - start_RR.tv_nsec) / 1e9);
     if (time_RR <= 0) {
          printf("ERROR: Zero or negative time");
          return 1;
     }

     //@Print out the read bandwidth
     mb = (double)bytesCount/1000000;
     printf("Read %f MB, use %d threads, elapsed time %f s, read bandwidth: %f MB/s \n", mb, p, time_RR, mb/time_RR);

     /* free up resources properly */
     free(buffer);

     return 0;
}
