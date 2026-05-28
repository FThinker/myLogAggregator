#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <time.h>
#include "protocol.h"
#include "network.h"

#define COLOR_RED   "\033[0;31m"
#define COLOR_GRAY "\033[0;90m"
#define COLOR_RESET "\033[0m"

int main() {
    // generate a random seed based on current time and PID
    srand(time(NULL) ^ getpid());

    printf(COLOR_GRAY "[Producer %d] Starting producer...\n" COLOR_RESET, getpid());

    int sock = connect_to_server(SERVER_IP, SERVER_PORT);
    printf(COLOR_GREEN "[Producer %d] Succesfully connected to coordinator.\n" COLOR_RESET, getpid());

    // Test message
    LogMessage msg;
    msg.sender_id = getpid();
    msg.data = ((double)rand() / RAND_MAX) * 1000.0; // random (0.00 -> 1000.00)

    // Test send
    if (send(sock, &msg, sizeof(LogMessage), 0) < 0) {
        perror(COLOR_RED "[Producer %d] Error whilst sending message" COLOR_RESET, getpid());
        exit(EXIT_FAILURE);
    }

    printf(COLOR_GRAY "[Producer %d] Message sent (ID: %d, Data: %.2f)!\n" COLOR_RESET, getpid(), msg.sender_id, msg.data);

    close(sock);
    return 0;
}