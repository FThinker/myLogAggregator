#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include "protocol.h"
#include "network.h"

int main() {
    printf("Coordinator listening on port %d...\n", SERVER_PORT);
    
    int server_fd = setup_server_socket(SERVER_PORT, MAX_BACKLOG);
    printf("Awaiting for...\n");

    // Accept ONE (1) connection
    int client_fd = accept(server_fd, NULL, NULL);
    if(client_fd < 0){
        printf("Error accepting connection.\n");
        exit(EXIT_FAILURE);
    }
    printf("Producer connected!\n");

    // Read message
    LogMessage msg;
    ssize_t bytes_read = recv(client_fd, &msg, sizeof(LogMessage), 0);
    
    if(bytes_read == sizeof(LogMessage) == -1) {
        printf("An error has occurred while receiving data.\n");
    }
    if(bytes_read == sizeof(LogMessage) == 0 ) {
        printf("Connection has been closed by peer");
    } 
    printf("Received: ID=%d, Data=%.2f\n", msg.sender_id, msg.data);

    // Close communications
    close(client_fd);
    close(server_fd);
    printf("Coordinator stopped.\n");
    
    return 0;
}