#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include "protocol.h"
#include "network.h"

int main() {
    printf("Starting producer...\n");

    int sock = connect_to_server(SERVER_IP, SERVER_PORT);
    printf("Succesfully connected to coordinator.\n");

    // Test message
    LogMessage msg;
    msg.sender_id = 42;
    msg.data = 123.45;

    // Test send
    if (send(sock, &msg, sizeof(LogMessage), 0) < 0) {
        perror("Error whilst sending message");
        exit(EXIT_FAILURE);
    }

    printf("Message sent (ID: %d, Data: %.2f)!\n", msg.sender_id, msg.data);

    close(sock);
    return 0;
}