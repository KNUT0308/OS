#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char** argv) {
  int page_size = getpagesize(); // Get page size for this system
  //printf("Page size: %d", page_size);
  
  int N = atoi(argv[1]); // Get input N, corresponding to number of pages to be allocated
  
  void *pages = malloc(N * page_size); // Dynamically allocate N pages (of the size page_size) in memory of unspecified type (void)

  if (pages == NULL) { // Error handling
    printf("malloc failed");
    return 1;
  }

  // Enable or disable the line below in order to run the program with or without instantiating the dynamically allocated space
  memset(pages, 0, N * page_size); // initialize all bytes of pages to 0 (fill all the space with zeros)

  free(pages); // Free allocated space on heap
  pages = NULL; // Avoid dangling pointers

  return 0;
}