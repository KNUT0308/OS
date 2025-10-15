#include <pthread.h> 
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdatomic.h>
     
int num_threads = 0;
     
typedef struct node { 
     int node_id;      //a unique ID assigned to each node
     struct node *next;
} Node;

typedef struct {
     int thread_id;
     int opt;
} ThreadArgs;


Node *top = NULL; // top for MUTEX stack
static int nid = 0; // node ID's for MUTEX stack
_Atomic(Node *) cas_top = NULL; //top for CAS stack
static atomic_int nid_cas = 0; // node ID's for CAS stack
pthread_mutex_t lock;

/*Option 1: Mutex Lock*/
void push_mutex() {
     Node *new_node = malloc(sizeof(Node)); 
     pthread_mutex_lock(&lock);
     new_node->node_id = nid++;
     new_node->next = top;
     top = new_node;
     pthread_mutex_unlock(&lock);


     //update top of the stack below
     //assign a unique ID to the new node
}

int pop_mutex() {
     pthread_mutex_lock(&lock);
     if (top == NULL){ //Empty stack
          pthread_mutex_unlock(&lock);
          return -1;
     }
     Node *old_node = top;
     int value = old_node->node_id;
     top = old_node->next;
     pthread_mutex_unlock(&lock);

     //update top of the stack below

     free(old_node); //Deallocate memory
     return value;
}

/*Option 2: Compare-and-Swap (CAS)*/
void push_cas() { 
     Node *new_node = malloc(sizeof(Node));

     new_node->node_id = atomic_fetch_add(&nid_cas, 1);
     Node *old_top;

     do {
          old_top = atomic_load(&cas_top);
          new_node->next = old_top;
     } while (!atomic_compare_exchange_weak(&cas_top, &old_top, new_node)); //Make cas_top = new_node if cas_top is equal to old_top.
                                                                           // Meaning to say if &cas_top is excpected and has not changed.
     


     //update top of the stack below
     //assign a unique ID to the new node
}

int pop_cas() { 
     Node *old_top;

     do {
          old_top = atomic_load(&cas_top); //get current cas_top.
          if (old_top == NULL){
               return -1;
          }
     } while (!atomic_compare_exchange_weak(&cas_top, &old_top, old_top->next));
     
     int value = old_top->node_id;
     free(old_top);
     return value;
     //update top of the stack below
}

/* the thread function */
void *thread_func(void *arg) { 
     /* Assign each thread an id so that they are unique in range [0, num_thread -1 ] */

     ThreadArgs *args = (ThreadArgs *)arg;
     int my_id = args->thread_id;
     int opt = args->opt;

     if( opt==0 ){
          push_mutex();push_mutex();pop_mutex();pop_mutex();push_mutex();
     }else{
          push_cas();push_cas();pop_cas();pop_cas();push_cas();
     }
     
     printf("Thread %d: exit\n", my_id);
     pthread_exit(0);
}

int main(int argc, char *argv[])
{
     num_threads = atoi(argv[1]);
     pthread_t *workers;
     workers = malloc(num_threads * sizeof(pthread_t)); //put this in myself.

     /* Option 1: Mutex */ 

     pthread_mutex_init(&lock, NULL); //initialize mutex lock
     ThreadArgs *args = malloc(num_threads * sizeof(ThreadArgs));

     for (int i = 0; i < num_threads; i++) { 
          pthread_attr_t attr;     
          pthread_attr_init(&attr);
          args[i].thread_id = i;
          args[i].opt = 0; // For MUTEX
          pthread_create(&workers[i], &attr, thread_func, &args[i]); 
     }
     for (int i = 0; i < num_threads; i++) {
     pthread_join(workers[i], NULL);
     } 

     //Print out all remaining nodes in Stack√
     printf("Mutex: Remaining nodes \n");
     Node *curr = top;
     while (curr != NULL){
          printf("Node ID: %d\n", curr->node_id);
          curr = curr->next;
     }

     free(workers);
     free(args);
     pthread_mutex_destroy(&lock);


     
     /*free up resources properly */

     printf("=============\n");
     
     /* Option 2: CAS */ 
     
     ThreadArgs *cas_args = malloc(num_threads * sizeof(ThreadArgs));
     pthread_t *cas_workers;
     cas_workers = malloc(num_threads * sizeof(pthread_t)); //put this in myself.

     for (int i = 0; i < num_threads; i++) { 
          pthread_attr_t attr;     
          pthread_attr_init(&attr);
          cas_args[i].thread_id = i;
          cas_args[i].opt = 1; // For CAS
          pthread_create(&cas_workers[i], &attr, thread_func, &cas_args[i]); 
     }
     for (int i = 0; i < num_threads; i++) {
     pthread_join(cas_workers[i], NULL);
     } 
     
     //Print out all remaining nodes in Stack
     printf("CAS: Remaining nodes \n");
     Node *cas_curr = cas_top;
     while (cas_curr != NULL){
          printf("Node ID: %d\n", cas_curr->node_id);
          cas_curr = cas_curr->next;
     }

     free(cas_workers);
     free(cas_args);
     
     /*free up resources properly */
     
     return 0;
}