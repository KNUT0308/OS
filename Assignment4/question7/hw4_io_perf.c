#include <pthread.h> 
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdatomic.h>

unsigned int bytesWritten;
unsigned int bytesRead;
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
          bytesRead += readTot;
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
          bytesWritten += writeTot;
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
          int character = fgetc(independence); // TODO: handle as "char" instead of "int" worked previously. Try if this gives error.
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


     // @start timing 
     bytesWritten = 0; // Ensure that our count of written bytes is 0 from the beginning
     struct timespec startSeq1, endSeq1;
     clock_gettime(CLOCK_MONOTONIC, &startSeq1);

     /* Create writer workers and pass in their portion of list1 */   
     pthread_t *writer_workers = malloc(p * sizeof(pthread_t)); // Space for thread IDs
     writer_args *argsW = malloc(p * sizeof(writer_args)); // Space for thread arguments
     int handled = 0; // Number of requests handled
     int piece = numSeq/p; // Optimization proposal: Give the first threads a bit more to do since they start earlier, instead of giving the last thread more than the others
     for (int i = 0; i < p; i++) {
          if (i == p-1 && handled < numSeq) { 
               piece = numSeq-handled; // Case when last thread and there is more left, then give the last thread all the remaining requests
          }
          argsW[i].requests = &sequential[handled];
          argsW[i].numRequests = piece;
          argsW[i].source = buffer;
          argsW[i].destination = fileDescriptor;
          pthread_create(&writer_workers[i], NULL, writer_thread_func, &argsW[i]);
          handled += piece;          
     }
     
     /* Wait for all writers to finish */ 
     for (int i = 0; i < p; i++) {
          pthread_join(writer_workers[i], NULL); // Make sure that each thread terminates before continuing
     }
     
     free(writer_workers);
     free(argsW);
     free(buffer);

     // @close the file 
     close(fileDescriptor);

     // @end timing 
     bytesRead = 0; // Ensure that our count of read bytes is 0 from the beginning
     clock_gettime(CLOCK_MONOTONIC, &endSeq1);
     double timeSeq1 = endSeq1.tv_sec - startSeq1.tv_sec + ((endSeq1.tv_nsec - startSeq1.tv_nsec) / 1e9); // Fetch amount of nanoseconds and convert to seconds
     if (timeSeq1 <= 0) {
          printf("ERROR: Zero or negative time");
          return 1;
     }

     //@Print out the write bandwidth
     double mb = (double)bytesWritten/1000000;
     printf("Write %f MB, use %d threads, elapsed time %f s, write bandwidth: %f MB/s \n", mb, p, timeSeq1, mb/timeSeq1);
     
     // @reopen the file 
     fileDescriptor = open("fileSeq.txt", O_RDONLY); // TODO: add ", 0644"?
     if (fileDescriptor < 0) {printf("ERROR: fileSeq.txt not opened for reading correctly"); return 1;}

     // @start timing 
     struct timespec startSeq2, endSeq2;
     clock_gettime(CLOCK_MONOTONIC, &startSeq2);

     /* Create reader workers and pass in their portion of list1 */   
     pthread_t *reader_workers = malloc(p * sizeof(pthread_t)); // Space for thread IDs
     reader_args *argsR = malloc(p * sizeof(reader_args)); // Space for thread arguments
     char* result = malloc(n); // Place to store what all the threads have read
     handled = 0; // Number of requests handled
     piece = numSeq/p; // Optimization proposal: Give the first threads a bit more to do since they start earlier, instead of giving the last thread more than the others
     for (int i = 0; i < p; i++) {
          if (i == p-1 && handled < numSeq) { 
               piece = numSeq-handled; // Case when last thread and there is more left, then give the last thread all the remaining requests
          }
          argsR[i].requests = &sequential[handled];
          argsR[i].numRequests = piece;
          argsR[i].result = result;
          argsR[i].source = fileDescriptor;
          pthread_create(&reader_workers[i], NULL, reader_thread_func, &argsR[i]);
          handled += piece;          
     }
     
     /* Wait for all reader to finish */ 
     for (int i = 0; i < p; i++) {
          pthread_join(reader_workers[i], NULL); // Make sure that each thread terminates before continuing
     }

     free(reader_workers);
     free(argsR);
     free(result);

     // @close the file 
     close(fileDescriptor);

     // @end timing 
     clock_gettime(CLOCK_MONOTONIC, &endSeq2);
     double timeSeq2 = endSeq2.tv_sec - startSeq2.tv_sec + ((endSeq2.tv_nsec - startSeq2.tv_nsec) / 1e9); // Fetch amount of nanoseconds and convert to seconds
     if (timeSeq2 <= 0) {
          printf("ERROR: Zero or negative time");
          return 1;
     }

     //@Print out the read bandwidth
     mb = (double)bytesRead/1000000;
     printf("Read %f MB, use %d threads, elapsed time %f s, read bandwidth: %f MB/s \n", mb, p, timeSeq2, mb/timeSeq2);


     // ****************** Write/read with List 2 (random) ******************


     // @Repeat the write and read test now using List2 


     /*free up resources properly */
     

     return 0;
}
