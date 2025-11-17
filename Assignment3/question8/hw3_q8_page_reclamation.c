#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct page {
    int page_id;
    int reference_bit;
    struct page* next;
    // @other auxiliary
} Node;

int n;
int m;
int* total_ref;
int done = 0;

// @create an active list
Node* activeListHead = NULL;
Node* activeListTail = NULL;
int activeListSize = 0;

// @create an inactive list
Node* inactiveListHead = NULL;
Node* inactiveListTail = NULL;
int inactiveListSize = 0;

// Mutex for the lists
pthread_mutex_t list_lock = PTHREAD_MUTEX_INITIALIZER;

void* player_thread_func(void* arg) {
    // The reference string    
    int* n_string = ((int*)arg);

    // Go through the reference string
    for (int i = 0; i < 1000; i++) {
        // Set up
        int currentID = n_string[i];
        int j = 0;
        Node* current = activeListHead;
        Node* found = NULL;

        if (current != NULL && current->page_id == currentID) {
            // The page is located in the head of the list
            found = current;
            // Remove head
            activeListHead = current->next;
            if (activeListHead == NULL) {
                // The list contains only one element
                activeListTail = NULL;
            }
            // Decrease the list size and set the reference bit to 1
            activeListSize--;
            found->reference_bit = 1;
            found->next = NULL;

        } else {
            // The page is inside the list (not the head)
            while (j < activeListSize && current -> next != NULL) {
                if (current->next->page_id == currentID) {
                    // We found the page. Take it out and set reference bit to 1
                    found = current->next;
                    // Unlink the node
                    current->next = current->next->next;
                    if (current->next == NULL) {
                        // There is nothing to link, current is last page
                        activeListTail = current;
                    }
                    activeListSize--;
                    found->reference_bit = 1;
                    found->next = NULL;
                    break;
                }
                // Next page in active list
                current = current->next;
                j++;
            }
        }
        if (found == NULL) {
            // The page is not in the active list, check the inactive list
            //
            // Set up
            current = inactiveListHead;
            j = 0;
            if (current != NULL && current->page_id == currentID) {
                // The page is located in the head of the list
                found = current;
                // Remove head
                inactiveListHead = current->next;
                if (inactiveListHead == NULL) {
                    // The list contains only one element
                    inactiveListTail = NULL;
                }
                // Decrease the list size and set the reference bit to 1
                inactiveListSize--;
                found->reference_bit = 1;
                found->next = NULL;
            } else {
                // The page is inside the list
                while (j < inactiveListSize && current -> next != NULL) {
                    if (current->next->page_id == currentID) {
                        // We found the page. Take it out and set reference bit to 1
                        found = current->next;
                        // Unlink the page
                        current->next = current->next->next;
                        if (current->next == NULL) {
                            // There is nothing to link, current is last page
                            inactiveListTail = current;
                        }
                        // Decrease the list size and set reference bit to 1
                        inactiveListSize--;
                        found->reference_bit = 1;
                        found->next = NULL;
                        break;
                    }
                    // Next page in inactive list
                    current = current->next;
                    j++;
                }
            }
        }
        if (found != NULL) {
            // Put last in the active list
            if (activeListHead == NULL) {
                // The active list is empty, put the found as the head and tail
                activeListHead = found;
                activeListTail = found;
                activeListSize++;
            } else {
                // The active list is not empty
                activeListTail -> next = found;
                activeListTail = found;
                activeListSize++;
            }
        
        } else {
            // CRAZY CASE
            printf("Whata hell boi i dont find the page bruh");
            exit(1);
        }

        // Sleep for 10us
        usleep(10);

        if (activeListSize > n * 0.7) {
            pthread_mutex_lock(&list_lock);
            // There is more than 70% of the capacity in the active list
            // Drain out the 20% of the most inactive pages in the active list to the inactive list
            Node* start = activeListHead;
            int limit = (int)(0.2 * activeListSize);
            Node* end = start;
            int moved = 1;
            while (moved < limit && end->next != NULL) {
                // Create the chuck of pages being moved
                end = end->next;
                moved++;
            }

            // Unlink the pages being moved from the active list
            activeListHead = end -> next;
            if (activeListHead == NULL) {
                activeListTail = NULL;
            }

            // Append the pages to the inactive list
            if (inactiveListTail == NULL) {
                // inactive list is empty
                inactiveListHead = start;
                inactiveListTail = end;
            } else {
                inactiveListTail->next = start;
                inactiveListTail = end;
            }
            // end is the last element in the inactive list
            inactiveListTail->next = NULL;

            // Update size
            activeListSize -= moved;
            inactiveListSize += moved;

            pthread_mutex_unlock(&list_lock);
        }
    }

    // The payer is done and can the checker can end as well
    done = 1;
    pthread_exit(0);
}

void* checker_thread_func(void* arg) {
    // Goes through the active list every M seconds and sets the reference bit to 0
    while (!done) {
        // Waits until the player is done
        pthread_mutex_lock(&list_lock);
        Node* current = activeListHead;
        while (current != NULL) {
            if (current->reference_bit == 1) {
                // Add the corresponding ref_bit by one
                total_ref[current->page_id]++;
            }
            // Clear the reference bit and go to the next page
            current->reference_bit = 0;
            current = current->next;
        }
        pthread_mutex_unlock(&list_lock);
        usleep(m);
    }

    pthread_exit(0);
}

// Generates a random int from min to max
int randfrom(int min, int max) {
    if (max < min) return min;
    return min + (rand() % (max - min + 1));

}

// Populates the inactive list with every int from 0 to N
void populate_inactive_list(int n, int* n_string) {
    for (int i = 0; i < n; i++) {
        Node* newNode = (Node*)malloc(sizeof(Node));
        if (newNode == NULL) {
            // Error handling
            printf("OMGOMGOMG");
            exit(1);
        }
        newNode->next = NULL;
        newNode->reference_bit = 0;
        newNode->page_id = i;

        // Put Node in place
        if (inactiveListHead == NULL) {
            // For an empty list
            inactiveListHead = newNode;
            inactiveListTail = newNode;

        } else {
            inactiveListTail->next = newNode;
            inactiveListTail = newNode;
        }
        inactiveListSize++;
    }
}


void populate_total_ref(int n) {
    total_ref = (int *) malloc(sizeof(int) * n);
    for (int i = 0; i < n; i++) {
        total_ref[i] = 0;
    }
}

int main(int argc, char* argv[]) {
    // Read from user
    n = atoi(argv[1]);
    m = atoi(argv[2]);
    // Init and populate the string reference
    srand(time(NULL));
    int* n_string = (int*)malloc(sizeof(int) * 1000);
    for (int i = 0; i < 1000; i++) {
      n_string[i] = randfrom(0, n - 1);
    }

    // Total ref is a counter for every reference bit set to 1
    populate_total_ref(n);

    // Create the inactive list with all the available values
    populate_inactive_list(n, n_string);

    /* Create two workers */
    pthread_t player;
    pthread_t checker;

    pthread_create(&player, NULL, player_thread_func, n_string);
    pthread_create(&checker, NULL, checker_thread_func, NULL);

    pthread_join(player, NULL);
    pthread_join(checker, NULL);

    printf("Page_Id, Total_Referenced\n");
    for (int i = 0; i < n; i++) {
        printf("%d, %d\n", i, total_ref[i]);
    }

    // Print active list
    printf("Pages in active list: ");
    Node* current = activeListHead;
    while (current != NULL) {
        if (current -> next != NULL) {
            printf("%d, ", current->page_id);
        } else {
            printf("%d", current->page_id);
        }
        current = current->next;
    }
    printf("\n");

    // Print inactive list
    printf("Pages in inactive list: ");
    current = inactiveListHead;
    while (current != NULL) {
        if (current -> next != NULL) {
            printf("%d, ", current->page_id);
        } else {
            printf("%d", current->page_id);
        }
        current = current->next;
    }
    printf("\n");

    /* free up resources properly */
    free(n_string);
    free(total_ref);

    // free nodes in active list
    current = activeListHead;
    while (current != NULL) {
        Node* temp = current->next;
        free(current);
        current = temp;
    }
    // free nodes in inactive list
    current = inactiveListHead;
    while (current != NULL) {
        Node* temp = current->next;
        free(current);
        current = temp;
    }
}
