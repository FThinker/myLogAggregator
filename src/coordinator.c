#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include "protocol.h"
#include "network.h"
#include "logger.h"
#include <bits/pthread_stack_min-dynamic.h>

#define COLOR_RED   "\033[0;31m"
#define COLOR_GRAY "\033[0;90m"
#define COLOR_ORANGE "\033[0;33m"
#define COLOR_GREEN "\033[0;32m"
#define COLOR_RESET "\033[0m"

typedef struct {
    pthread_t thread_id;
    int is_busy;      // 0 = free, 1 = busy
    int needs_join;   // 1 = thread is done, must be cleared by joining
    int client_fd;
} PoolSlot;

bool debug_mode = false;

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
    if(debug_mode) printf("[Thread %lu] Started handling new producer.\n", pthread_self());

    LogMessage msg;
    int current_sender_id = -1;
    
    // 2. Loop read until the client disconnects or an error occurs
    while (1) {
        ssize_t bytes_read = recv(client_fd, &msg, sizeof(LogMessage), 0);
        
        if (bytes_read == -1) { // Generic error during recv
            if(debug_mode) perror(COLOR_RED "Error while receiving data" COLOR_RESET);
            break;
        } 
        else if (bytes_read == 0) { // Client closed the connection
            if(debug_mode) printf("[Thread %lu] Producer has closed the communication (EOF).\n", pthread_self());
            if(current_sender_id != -1) {
                logger_write_disconnect(current_sender_id);
            }
            break;
        } 
        else if (bytes_read == sizeof(LogMessage)) { // Successful read
            current_sender_id = msg.sender_id;
            if(debug_mode) printf("[Thread %lu] Received: ID=%d, Data=%.2f\n", pthread_self(), msg.sender_id, msg.data);

            logger_write_data(msg.sender_id, msg.data);
        }
         else { // Partial read, which shouldn't happen with TCP if the message is small enough, but we handle it just in case
            fprintf(stderr, "Partial message received. Expected %lu bytes, got %zd bytes.\n", sizeof(LogMessage), bytes_read);
            break;
        }
    }

    // 3. Closed specific socket of this client and thread died 
    // using mutex because main might be looking for a slot in the meantime
    pthread_mutex_lock(&pool_mutex);
    thread_pool[my_slot].is_busy = 0;
    thread_pool[my_slot].needs_join = 1;
    pthread_mutex_unlock(&pool_mutex);
    
    return NULL;
}


int main(int argc, char *argv[]) {
    // check for debug flag
    if (argc > 1 && (strcmp(argv[1], "-d") == 0 || strcmp(argv[1], "--debug") == 0)) {
        debug_mode = true;
        printf(COLOR_GREEN "[INFO] Debug mode enabled. Output will be verbose.\n" COLOR_RESET);
    }

    if (!logger_init(LOG_FILE_NAME)) {
        fprintf(stderr, COLOR_RED "Failed to initialize logger. Exiting.\n" COLOR_RESET);
        exit(EXIT_FAILURE);
    }

    init_pool();

    // manually tweaking thread attributes to lower memory impact
    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    // Stack size is the minimum allowed by OS (here, 16KB) + 32KB as a safety buffer
    size_t minimal_stack_size = PTHREAD_STACK_MIN + 32768; 
    pthread_attr_setstacksize(&thread_attr, minimal_stack_size);

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
                perror(COLOR_RED "Error creating thread" COLOR_RESET);
                thread_pool[free_slot].is_busy = 0; // reset slot on failure
                free(slot_ptr);
                close(client_fd);
            }
        } else {
            // if no free slots are found, refuse any further connection request until a new slow is available
            printf(COLOR_ORANGE "Server is full, refusing the connection.\n" COLOR_RESET);
            close(client_fd);
        }

        // after checking for a free slot and potentially creating a thread, we can release the lock
        pthread_mutex_unlock(&pool_mutex);
        
    }

    // Close communications
    pthread_attr_destroy(&thread_attr);
    close(server_fd);
    logger_close();
    printf(COLOR_GRAY "Coordinator stopped.\n" COLOR_RESET);
    
    return 0;
}