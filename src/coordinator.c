#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>
#include "protocol.h"
#include "network.h"


pthread_t active_threads[MAX_BACKLOG];
int thread_count = 0;

// worker function used by each thread
void* worker_thread(void *arg) {
    // 1. Retrieve socket from argument and free memory to avoid leaks
    int client_fd = *((int *)arg);
    free(arg); 

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
    close(client_fd);
    printf("[Thread %lu] Terminated thread.\n", pthread_self());
    //thread_count--;
    
    return NULL;
}


int main() {
    printf("Coordinator listening on port %d...\n", SERVER_PORT);
    
    int server_fd = setup_server_socket(SERVER_PORT, MAX_BACKLOG);
    printf("Awaiting for...\n");

    // Accept ONE (1) connection
    // int client_fd = accept(server_fd, NULL, NULL);
    // if(client_fd < 0){
    //     printf("Error accepting connection.\n");
    //     exit(EXIT_FAILURE);
    // }
    // printf("Producer connected!\n");
    while(1){
        int client_fd = accept(server_fd, NULL, NULL);
        if(client_fd < 0){
            printf("Error accepting connection.\n");
            continue;
        }

        printf("Producer connected! Assigning a thread... \n");

        // dynamically allocate memory for client_fd to avoid race conditions
        int *client_fd_ptr = malloc(sizeof(int));
        if (client_fd_ptr == NULL) {
            perror("Error whilst allocating memory");
            close(client_fd);
            continue;
        }
        *client_fd_ptr = client_fd;

        if(thread_count < MAX_BACKLOG){
            if(pthread_create(&active_threads[thread_count], NULL, worker_thread, client_fd_ptr) != 0){
                perror("Error creating thread");
                free(client_fd_ptr);
                close(client_fd);
            } 
            else {
                thread_count++;
            }
        } else {
            printf("Maximum number of connections reached. Rejecting new connection.\n");
            free(client_fd_ptr);
            close(client_fd);
        }
        
    }

    // Close communications
    close(server_fd);
    printf("Coordinator stopped.\n");
    
    return 0;
}