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

    int sock = -1;

    while (sock < 0) { // keep trying to connect until successful
        sock = connect_to_server(SERVER_IP, SERVER_PORT);
        if (sock < 0) {
            printf(COLOR_ORANGE "[Producer %d] Connection failed/refused. Retrying in 1s...\n" COLOR_RESET, getpid());
            sleep(1);
        }
    }

    printf(COLOR_GREEN "[Producer %d] Succesfully connected to coordinator.\n" COLOR_RESET, getpid());

    // Random number of messages to send (1-5)
    int num_messages = (rand() % 5) + 1;

    for (int i = 0; i < num_messages; i++) {
        LogMessage msg;
        msg.sender_id = getpid();
        msg.data = ((double)rand() / RAND_MAX) * 1000.0; // random (0.00 -> 1000.00)

        // try to send
        if (send(sock, &msg, sizeof(LogMessage), 0) < 0) {
            perror(COLOR_RED "[Producer %d] Error whilst sending message" COLOR_RESET, getpid());
            break; // if connection blows up, stop trying to send more messages
        }

        printf(COLOR_GRAY "[Producer %d] Message %d/%d sent (Data: %.2f)!\n" COLOR_RESET, getpid(), i+1, num_messages, msg.data);

        // random delay between 500ms and 1500ms before sending the next message
        // (only if there are more messages to send, otherwise we can just disconnect immediately)
        if (i < num_messages - 1) {
            int delay_ms = 500 + (rand() % 1000); // 500 + (0 to 999)
            usleep(delay_ms * 1000); // usleep uses microseconds, so conversion is needed
        }
    }

    printf(COLOR_GRAY "[Producer %d] Finished sending data. Disconnecting.\n" COLOR_RESET, getpid());
    close(sock);
    return 0;
}