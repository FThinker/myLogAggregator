#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <time.h>
#include "protocol.h"
#include "network.h"

int main() {
    // generate a random seed based on current time and PID
    srand(time(NULL) ^ getpid());

    printf("Starting producer (PID: %d)...\n", getpid());

    int sock = connect_to_server(SERVER_IP, SERVER_PORT);
    printf("Succesfully connected to coordinator.\n");

    // Test message
    LogMessage msg;
    msg.sender_id = getpid();
    msg.data = ((double)rand() / RAND_MAX) * 1000.0; // random (0.00 -> 1000.00)

    // Test send
    if (send(sock, &msg, sizeof(LogMessage), 0) < 0) {
        perror("Error whilst sending message");
        exit(EXIT_FAILURE);
    }

    printf("Message sent (ID: %d, Data: %.2f)!\n", msg.sender_id, msg.data);

    close(sock);
    return 0;
}