#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>

int main(int argc, char** argv) {
    struct timespec start, end;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <num_pages>\n", argv[0]);
        exit(1);
    }

    int num_pages = atoi(argv[1]);
    int page_size = getpagesize();
    int flags;

    printf("Allocating %d pages of %d bytes \n", num_pages, page_size);

    char* addr;
    // OPTION 1
    

    //flags = MAP_ANONYMOUS | MAP_PRIVATE;
    // OPTION 2
    flags = MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB;
    // @Add the start of Timer here
    clock_gettime(CLOCK_MONOTONIC, &start); 
    

    addr = (char*) mmap(NULL, num_pages* page_size, PROT_WRITE | PROT_READ, flags, -1, 0);

    if (addr == MAP_FAILED) {
        printf("OMGOMGOMG");
        perror("mmap");
        exit(1);
    }

    // the code below updates the pages
    char c = 'a';
    for (int i = 0; i < num_pages; i++) {
        addr[i * page_size] = c;
        c++;
    }

    // @Add the end of Timer here
    clock_gettime(CLOCK_MONOTONIC, &end);

    double time_elapsed = (end.tv_nsec - start.tv_nsec) / 1e6;

    // @Add printout of elapsed time in cycles
    printf("%.9f ms time elapsed\n", time_elapsed);

    for (int i = 0; (i < num_pages && i < 16); i++) {
        printf("%c ", addr[i * page_size]);
    }
    printf("\n");

    munmap(addr, page_size * num_pages);
}

