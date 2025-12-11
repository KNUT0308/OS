#include <pthread.h> 
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
          

void *reader_thread_func(/*@input paramters*/) { 
     
     // @Add code for reader threads
     // @Given a list of [offset1, bytes1], [offset2, bytes2], ...
     // @for each: read bytes_i from offset_i

     pthread_exit(0);
}


void *writer_thread_func(int* offsets, int bytes) { 
     
     // @Add code for writer threads
     // @Given a list of [offset1, bytes1], [offset2, bytes2], ...
     // @for each: write bytes_i to offset_i

     pthread_exit(0);
}


int main(int argc, char *argv[])
{
     int n = atoi(argv[1]); // number of bytes
     int p = atoi(argv[2]); // number of threads

     // @create a file for saving the data
     FILE* file;
     file = fopen("file.txt", "w"); // Open a file in write-mode

     // @allocate a buffer and initialize it
     char* buffer = malloc(n); // Dynamically allocate N bytes in memory of type char, to be used as temporary storage space
     
     FILE* independence;
     independence = fopen("congress-07-04-1776.txt", "r"); // Open the Declaration of Independence in read-mode
     
     if (file == NULL || independence == NULL) {
          printf("ERROR: File not handled correctly");
          return 1;
     }
     
     char character = fgetc(independence);
     for (int i = 0; i < n; i++) {
          if (character != EOF) {
               buffer[i] = character; // Write n bytes (characters) of the Declaration of Independence to the buffer
               character = fgetc(independence);
          } else {
               buffer[i] = 'x';
          }
     }

     fclose(independence);
     
     // @create two lists of 100 requests in the format of [offset, bytes]

     // @List 1: sequtial requests of 16384 bytes, where offset_n = offset_(n-1) + 16384
     // @e.g., [0, 16384], [16384, 16384], [32768, 16384] ...
     // @ensure no overlapping among these requests.

     int requestsSequential[100]; // Only contains the offsets [0, 16384, 32768, ...] and should be combined with the constant byte value 16384
     int offset = 0;
     for (int i = 0; i < sizeof(requestsSequential)/sizeof(requestsSequential[0]); i++) {
          requestsSequential[i] = offset;
          offset += 16384;
     }

     // @List 2: random requests of 128 bytes, where offset_n = random[0,N/4096] * 4096
     // @e.g., [4096, 128], [16384, 128], [32768, 128], etc.
     // @ensure no overlapping among these requests.

     int requestsRandom[100]; // Only contains the random offsets [4096, 16384, 32768, ...] and should be combined with the constant byte value 128
     int random = 0;
     if (n/4096 > 1) {
          random = (rand() % (n/4096)) * 4096;
     } else {
          random = rand() * 4096;
     }
     for (int i = 0; i < sizeof(requestsRandom)/sizeof(requestsRandom[0]); i++) {
          requestsRandom[i] = random;
          random = (rand() % (n/4096)) * 4096;
     }

     // @start timing 

     // TODO (above)

     /* Create writer workers and pass in their portion of list1 */   
     pthread_t *workers = malloc(p * sizeof(pthread_t));
     int *ids = malloc(p * sizeof(int));

     for (int i = 0; i < p; i++) {
          //pthread_attr_t attr;
          //pthread_attr_init(&attr);
          //ids[i] = i;
          pthread_create(&workers[i], NULL, writer_thread_func, &requestsSequential[i]); // Creates a thread with ID &workers[i], that runs the writer_thread_func function for the 128 bytes after &requestsSequential[i] in the array of requests
          // TODO: Behöver tillse att Byte storleken (antingen 16394 eller 128) även förs med som argument. Gör via global variabel eller genom att smyga in den siffran som första element i array:en som indikator?
     }
     
     /* Wait for all writers to finish */ 
     

     // @close the file 


     // @end timing 


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
     free(buffer);

     return 0;
}
