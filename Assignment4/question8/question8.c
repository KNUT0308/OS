#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

void readAndWrite(pid_t pid, void* mmaped_ptr) {
    if (pid == 0) {
        // Child process
        //
        // Child message
        char* message = "01234";
        // Memory mapping
        printf("Child process (pid=%d); mmap address %p\n", pid, mmaped_ptr);

        // Memory address for child write
        int* child_target = (int*)(char*)mmaped_ptr + 0;

        memcpy(child_target, message, strlen(message));

        msync(child_target, strlen(message), MS_SYNC);
        sleep(1);

        // Memory address for the parent write
        char* parent_target = (char*)(mmaped_ptr + 4096);

        printf("child process (pid=%d); read from mmaped_ptr[4096] %s\n", pid, parent_target);

    } else {
        // Parent process
        //
        // Parent message
        char* message = "56789";
        // Memory mapping
        printf("Parent process (pid=%d); mmap address %p\n", pid, mmaped_ptr);

        // Write message to parent address
        int* parent_target = (int*)(char*)(mmaped_ptr + 4096);

        memcpy(parent_target, message, strlen(message));
        msync(parent_target, strlen(message), MS_SYNC);
        sleep(1);
        // Get message from child address
        char* child_target = (char*)mmaped_ptr + 0;
        printf("Parent process (pid=%d); read from mmaped_ptr[0] %s\n", pid, child_target);
    }
}

void runProcesses() {
    int size = 4096 + 10;

    int fd = open("./file_to_map.txt", O_RDWR | O_TRUNC | O_CREAT);
    if (fd == -1) {
        perror("FD ERROR");
        exit(1);
    }
    if (ftruncate(fd, size) == -1) {
        perror("TRUNCADE ERROR");
        exit(1);
    }

    pid_t pid = fork();

    if (pid < 0) {
        printf("Fork error");
        exit(0);
    }

    void* mmaped_ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    readAndWrite(pid, mmaped_ptr);

    // Close
    munmap(mmaped_ptr, size);
    close(fd);
}

int main() {
    runProcesses();
    return 0;
}
