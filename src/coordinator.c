#define _XOPEN_SOURCE 700
#include <errno.h> 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <signal.h>
#include "protocol.h"
#include "network.h"
#include "logger.h"
#include <bits/pthread_stack_min-dynamic.h>

#define COLOR_RED   "\033[0;31m"
#define COLOR_GRAY "\033[0;90m"
#define COLOR_ORANGE "\033[0;33m"
#define COLOR_GREEN "\033[0;32m"
#define COLOR_RESET "\033[0m"


// ######################################################################################### //
//                                 DATA STRUCTURES AND GLOBALS                               //
// ######################################################################################### //


#define ALARM_INTERVAL 1 // timer is 5 seconds

// Thread pool slot structure
typedef struct {
    pthread_t thread_id;
    int is_busy;      // 0 = free, 1 = busy
    int needs_join;   // 1 = thread is done, must be cleared by joining
    int client_fd;
} PoolSlot;

bool debug_mode = false; // flag for verbose output

PoolSlot thread_pool[MAX_BACKLOG];
pthread_mutex_t pool_mutex = PTHREAD_MUTEX_INITIALIZER; // mutex to guard access to the thread pool

volatile sig_atomic_t keep_running = 1; // flag for the main loop, set to 0 when SIGINT is received to start shutdown procedure
volatile sig_atomic_t check_log_size = 0; // flag to check log size, set to 1 when SIGALARM triggers
// volatile is used to avoid compiler optimization such as caching, we need the value as fresh as possible


// ######################################################################################### //
//                                        FUNCTIONS                                          //
// ######################################################################################### //


void handle_sigint(int sig) {
    keep_running = 0; // SIGINT makes main loop stop accepting new connections and start shutdown procedure
}


// ----------------------------------------------------------------------------------------- //


void handle_sigalrm(int sig) {
    check_log_size = 1; // SIGALARM makes main loop check the file for rotation
}


// ----------------------------------------------------------------------------------------- //


void handle_sigpipe(int sig) {
    // SIGPIPE gets handled here, but actually it never happens
    // The disconnection happens when recv() returns 0.
    // also, printf isnt safe inside handlers so it's better to use write    
    const char *msg = "\nSIGPIPE detected.\n";
    write(STDOUT_FILENO, msg, strlen(msg));
}


// ----------------------------------------------------------------------------------------- //


// thread pool initialization
void init_pool() {
    for (int i = 0; i < MAX_BACKLOG; i++) {
        thread_pool[i].is_busy = 0;
        thread_pool[i].needs_join = 0;
    }
}


// ----------------------------------------------------------------------------------------- //


void print_usage(const char* prog_name) {
    printf("\n");
    printf("======================= Coordinator ====================\n");
    printf(COLOR_GRAY);
    printf("Usage: %s [-d]\n", prog_name);
    printf("Options:\n");
    printf("  -d, --debug   Enable debug mode\n");
    printf(COLOR_RESET);
    printf("========================================================\n");   
    printf("\n");
}


// ######################################################################################### //
//                                          THREAD                                           //
// ######################################################################################### //


void* worker_thread(void* arg) {
    // 1. Retrieve socket from argument and free memory to avoid leaks
    int my_slot = *((int*)arg);
    free(arg); 

    int client_fd = thread_pool[my_slot].client_fd;
    if(debug_mode) printf(COLOR_ORANGE "[Thread %lu] Started handling new producer.\n" COLOR_RESET, pthread_self());

    LogMessage msg;
    int current_sender_id = -1;
    
    // 2. Loop read until the client disconnects or an error occurs
    while (1) {
        ssize_t bytes_read = recv(client_fd, &msg, sizeof(LogMessage), 0);
        
        if (bytes_read == -1) { // Generic error during recv
            if (errno == EINTR) continue;

            if(debug_mode) perror(COLOR_RED "[Coordinator] Error while receiving data" COLOR_RESET);
            break;
        } 
        else if (bytes_read == 0) { // Client closed the connection
            if(debug_mode) printf(COLOR_ORANGE "[Thread %lu] Producer has closed the communication (EOF).\n" COLOR_RESET, pthread_self());
            if(current_sender_id != -1) {
                // the real disconnections happens here (RIP SIGPIPE)
                logger_write_disconnect(current_sender_id);
            }
            break;
        } 
        else if (bytes_read == sizeof(LogMessage)) { // Successful read
            current_sender_id = msg.sender_id;
            if(debug_mode) printf(COLOR_GRAY "[Thread %lu] Received: ID=%d, Data=%.2f\n" COLOR_RESET, pthread_self(), msg.sender_id, msg.data);

            logger_write_data(msg.sender_id, msg.data);
        }
         else { // Partial read SHOULDN'T happen with TCP if the message is small enough, but we handle it just in case
            fprintf(stderr, COLOR_RED "[Coordinator] Partial message received. Expected %lu bytes, got %zd bytes.\n" COLOR_RESET, sizeof(LogMessage), bytes_read);
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


// ######################################################################################### //
//                                           MAIN                                            //
// ######################################################################################### //


int main(int argc, char *argv[]) {
    // check for debug flag
    if (argc > 1 && (strcmp(argv[1], "-d") == 0 || strcmp(argv[1], "--debug") == 0)) {
        debug_mode = true;
        printf(COLOR_GREEN "[INFO] Debug mode enabled. Output will be verbose.\n" COLOR_RESET);
    } else if (argc > 1) {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    // signal handlers setup using sigaction
    struct sigaction sa;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    sa.sa_handler = handle_sigint;
    sigaction(SIGINT, &sa, NULL);

    sa.sa_handler = handle_sigalrm;
    sigaction(SIGALRM, &sa, NULL);

    sa.sa_handler = handle_sigpipe;
    sigaction(SIGPIPE, &sa, NULL);

    if (!logger_init(LOG_FILE_NAME)) {
        fprintf(stderr, COLOR_RED "[Coordinator] Failed to initialize logger. Exiting.\n" COLOR_RESET);
        exit(EXIT_FAILURE);
    }

    init_pool();

    // manually tweaking thread attributes to lower memory impact
    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    // Stack size is the minimum allowed by OS (here, 16KB) + 32KB as a safety buffer
    size_t minimal_stack_size = PTHREAD_STACK_MIN + 32768; 
    pthread_attr_setstacksize(&thread_attr, minimal_stack_size);

    printf(COLOR_GRAY "[Coordinator] Listening on port %d...\n" COLOR_RESET, SERVER_PORT);
    int server_fd = setup_server_socket(SERVER_PORT, MAX_BACKLOG);

    printf(COLOR_GRAY "[Coordinator] Awaiting connections...\n" COLOR_RESET);

    alarm(ALARM_INTERVAL); // start the timer for log checks

    while(keep_running){
        // check if alarm is up
        if (check_log_size) {
            if(debug_mode) printf(COLOR_GRAY "[Coordinator] Checking log size...\n" COLOR_RESET);
            logger_check_and_rotate(MAX_LOG_SIZE);
            check_log_size = 0;
            alarm(ALARM_INTERVAL); // Reset the alarm for the next check
        }

        int client_fd = accept(server_fd, NULL, NULL);
        if(client_fd < 0){
            // If error is caused by a signal interrupt, just continue and try again
            if (errno == EINTR) {
                continue; 
            }
            printf(COLOR_RED "[Coordinator] Error accepting connection.\n" COLOR_RESET);
            continue;
        }

        printf(COLOR_GRAY "[Coordinator] Producer %d connected! Assigning a thread... \n" COLOR_RESET, client_fd);

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
                perror(COLOR_RED "[Coordinator] Error creating thread" COLOR_RESET);
                thread_pool[free_slot].is_busy = 0; // reset slot on failure
                free(slot_ptr);
                close(client_fd);
            }
        } else {
            // if no free slots are found, refuse any further connection request until a new slow is available
            printf(COLOR_ORANGE "[Coordinator] Server is full, refusing the connection.\n" COLOR_RESET);
            close(client_fd);
        }

        // after checking for a free slot and potentially creating a thread, we can release the lock
        pthread_mutex_unlock(&pool_mutex);
        
    }

    // graceful shutdown
    printf(COLOR_ORANGE "\n[SHUTDOWN] Interrupt received. Starting controlled shutdown...\n" COLOR_RESET);
    
    // Close listening sockets
    close(server_fd);
    printf(COLOR_GRAY "[SHUTDOWN] Socket closed.\n" COLOR_RESET);

    // Wait for all threads to finish writing
    for (int i = 0; i < MAX_BACKLOG; i++) {
        if (thread_pool[i].is_busy == 1 || thread_pool[i].needs_join == 1) {
            if(debug_mode) printf(COLOR_GRAY "[SHUTDOWN] Waiting for slot %d...\n" COLOR_RESET, i);
            pthread_join(thread_pool[i].thread_id, NULL);
        }
    }
    printf(COLOR_GREEN "[SHUTDOWN] All threads terminated.\n" COLOR_RESET);

    // Close log file and cleanup
    pthread_attr_destroy(&thread_attr);
    logger_close();
    printf(COLOR_GREEN "[SHUTDOWN] Coordinator stopped succesfully.\n" COLOR_RESET);
    
    return 0;
}


// ----------------------------------------------------------------------------------------- //