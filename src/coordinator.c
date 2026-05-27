#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>
#include "protocol.h"
#include "network.h"

typedef struct {
    pthread_t thread_id;
    int is_busy;      // 0 = free, 1 = busy
    int needs_join;   // 1 = thread is done, must be cleared by joining
    int client_fd;
} PoolSlot;

PoolSlot thread_pool[MAX_BACKLOG];
int thread_count = 0;
pthread_mutex_t pool_mutex = PTHREAD_MUTEX_INITIALIZER;

// initialize the thread pool
void init_pool() {
    for (int i = 0; i < MAX_BACKLOG; i++) {
        thread_pool[i].is_busy = 0;
        thread_pool[i].needs_join = 0;
    }
}

// worker function used by each thread
void* worker_thread(void* arg) {
    // 1. Retrieve socket from argument and free memory to avoid leaks
    int my_slot = *((int*)arg);
    free(arg); 

    int client_fd = thread_pool[my_slot].client_fd;
    printf("[Thread %lu] Started handling new producer.\n", pthread_self());

    LogMessage msg;
    
    // 2. Loop read until the client disconnects or an error occurs
    while (1) {
        ssize_t bytes_read = recv(client_fd, &msg, sizeof(LogMessage), 0);
        
        if (bytes_read == -1) { // Generic error during recv
            perror("Error while receiving data");
            break;
        } 
        else if (bytes_read == 0) { // Client closed the connection
            printf("[Thread %lu] Producer has closed the communication (EOF).\n", pthread_self());
            break;
        } 
        else if (bytes_read == sizeof(LogMessage)) { // Successful read
            printf("[Thread %lu] Received: ID=%d, Data=%.2f\n", pthread_self(), msg.sender_id, msg.data);
        } // messaggio parziale?
    }

    // 3. Closed specific socket of this client and thread died 
    // using mutex because main might be looking for a slot in the meantime
    pthread_mutex_lock(&pool_mutex);
    thread_pool[my_slot].is_busy = 0;
    thread_pool[my_slot].needs_join = 1;
    pthread_mutex_unlock(&pool_mutex);
    
    return NULL;
}


int main() {
    init_pool();
    printf("Coordinator listening on port %d...\n", SERVER_PORT);
    
    int server_fd = setup_server_socket(SERVER_PORT, MAX_BACKLOG);
    printf("Awaiting connections...\n");

    while(1){
        int client_fd = accept(server_fd, NULL, NULL);
        if(client_fd < 0){
            printf("Error accepting connection.\n");
            continue;
        }

        printf("Producer connected! Assigning a thread... \n");

        // look for a free slot while locking to avoid race conditions
        pthread_mutex_lock(&pool_mutex);
        int free_slot = -1;
        for (int i = 0; i < MAX_BACKLOG; i++) {
            if (thread_pool[i].is_busy == 0) {
                free_slot = i;
                break;
            }
        }

        if (free_slot != -1) {
            // If the slot was previously used it needs to be joined before assigning the slot to a new thread
            if (thread_pool[free_slot].needs_join == 1) {
                pthread_mutex_unlock(&pool_mutex); // release lock before joining
                pthread_join(thread_pool[free_slot].thread_id, NULL);
                pthread_mutex_lock(&pool_mutex); // immediately reacquire it right after
                
                thread_pool[free_slot].needs_join = 0;
            }

            // Occupy the slot with the new thread
            thread_pool[free_slot].is_busy = 1;
            thread_pool[free_slot].client_fd = client_fd;
            
            int *slot_ptr = malloc(sizeof(int));
            *slot_ptr = free_slot; // Pass the slot index to the thread function

            if (pthread_create(&thread_pool[free_slot].thread_id, NULL, worker_thread, slot_ptr) != 0) {
                perror("Error creating thread");
                thread_pool[free_slot].is_busy = 0; // reset slot on failure
                free(slot_ptr);
                close(client_fd);
            }
        } else {
            // if no free slots are found, refuse any further connection request until a new slow is available
            printf("Server is full, refusing the connection.\n");
            close(client_fd);
        }

        // after checking for a free slot and potentially creating a thread, we can release the lock
        pthread_mutex_unlock(&pool_mutex);
        
    }

    // Close communications
    close(server_fd);
    printf("Coordinator stopped.\n");
    
    return 0;
}