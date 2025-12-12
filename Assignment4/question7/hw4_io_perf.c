#include <pthread.h> 
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

typedef struct {
     int offset;
     int bytes;
} request; // Nickname "request"

typedef struct {
     request *requests;
     int numRequests;
     char* source; // What to write
     int destination; // Where to write, using a file descriptor
} writer_args; // Nickname "writer_args"

void *reader_thread_func(/* ADD */) { 
     // @Add code for reader threads
     // @Given a list of [offset1, bytes1], [offset2, bytes2], ...
     // @for each: read bytes_i from offset_i

     // TODO

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

     int offset = 0;
     int bytes = 0;
     int written = 0;
     int write = 0;
     for (int i = 0; i < numRequests; i++) {
          offset = requests[i].offset;
          bytes = requests[i].bytes;
          char *source = buffer + offset;

          while(written < bytes) { // Loop until all bytes written
               write = pwrite(fileDescriptor, source + written, bytes - written, offset + written);
               if (write < 0) {printf("ERROR: write failed"); pthread_exit(NULL);}
               written += write;
          }
          written = 0;
     }     
     pthread_exit(0);
}


int main(int argc, char *argv[])
{
     int n = atoi(argv[1]);
     int p = atoi(argv[2]);

     // @create a file for saving the data
     int fileDescriptor = open("file.txt", O_WRONLY | O_CREAT);
     if (fileDescriptor < 0) {printf("ERROR: file.txt not handled correctly"); return 1;}

     // @allocate a buffer and initialize it
     char* buffer = malloc(n); // Dynamically allocate N bytes in memory of type char, to be used as temporary storage space
     
     FILE* independence;
     independence = fopen("congress-07-04-1776.txt", "r"); // Open the Declaration of Independence in read-mode
     if (independence == NULL) { printf("ERROR: congress-07-04-1776.txt not handled correctly"); return 1;}
     
     char character = fgetc(independence);
     for (int i = 0; i < n; i++) {
          if (character != EOF) {
               buffer[i] = character; // Write n bytes (characters) of the Declaration of Independence to the buffer
               character = fgetc(independence);
          } else {
               buffer[i] = 'x'; // If End-Of-File, write 'x' instead
          }
     }

     fclose(independence);
     
     // @create two lists of 100 requests in the format of [offset, bytes]
     
     // @List 1: sequtial requests of 16384 bytes, where offset_n = offset_(n-1) + 16384
     // @e.g., [0, 16384], [16384, 16384], [32768, 16384] ...
     // @ensure no overlapping among these requests.

     request sequential[100];
     int offset = 0;
     for (int i = 0; i < 100 && offset + 16384 <= n; i++) {
          sequential[i].offset = offset;
          sequential[i].bytes = 16384;
          offset += 16384;
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

     int j = 0;
     int tmp = 0;
     for (int i = pages-1; i > 0; i--) {
          j = rand() % (i+1); // Randomly select an index smaller than the current index i
          tmp = index[i]; // Swap the current index i with index j
          index[i] = index[j];
          index[j] = tmp;
     }

     request random[100];
     for (int i = 0; i < 100 && i < (int)pages; i++) {
          random[i].offset = index[i] * 4096;
          random[i].bytes = 128;
     }

     free(index);

     // @start timing 
     struct timespec start, end;
     double timeSequential = 0;
     clock_gettime(CLOCK_MONOTONIC, &start);

     /* Create writer workers and pass in their portion of list1 */   
     pthread_t *workers = malloc(p * sizeof(pthread_t)); // space for thread IDs
     writer_args *args = malloc(p * sizeof(writer_args)); // space for thread arguments
     int handled = 0;
     int piece = 100/p; // Optimization proposal: Give the first threads a bit more to do since they start earlier, instead of giving the last thread more than the others
     for (int i = 0; i < p; i++) {
          if (i == p-1 && handled < 100) { // Case when last thread and there is more left
               piece = 100-handled;     
          }
          args[i].requests = &sequential[handled];
          args[i].numRequests = piece;
          args[i].source = buffer;
          args[i].destination = fileDescriptor;
          pthread_create(&workers[i], NULL, writer_thread_func, &args[i]);
          handled += piece;          
     }
     
     /* Wait for all writers to finish */ 
     for (int i = 0; i < p; i++) {
          pthread_join(workers[i], NULL); // Make sure that each thread terminates before continuing
     }
     
     free(workers);
     free(args);
     free(buffer);

     // @close the file 
     close(fileDescriptor);

     // @end timing 
     clock_gettime(CLOCK_MONOTONIC, &end);
     timeSequential = (end.tv_nsec - start.tv_nsec) / 1e6;

     //@Print out the write bandwidth
     //printf("Write %f MB, use %d threads, elapsed time %f s, write bandwidth: %f MB/s \n", /**/);
     
     
     // @reopen the file 

     // @start timing 

     /* Create reader workers and pass in their portion of list1 */   
     
     
     /* Wait for all reader to finish */ 
     

     // @close the file 


     // @end timing 


     //@Print out the read bandwidth
     //printf("Read %f MB, use %d threads, elapsed time %f s, write bandwidth: %f MB/s \n", /**/);


     // @Repeat the write and read test now using List2 


     /*free up resources properly */


     return 0;
}
