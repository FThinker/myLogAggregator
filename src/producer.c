#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <time.h>
#include "protocol.h"
#include "network.h"

#define COLOR_RED   "\033[0;31m"
#define COLOR_GRAY "\033[0;90m"
#define COLOR_ORANGE "\033[0;33m"
#define COLOR_GREEN "\033[0;32m"
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
        msg.data = ((double)rand() / RAND_MAX) * 1000.0; 

        int message_sent = 0;

        // keep trying until message is sent succesfully
        while (!message_sent) {
            // MSG_NOSIGNAL prevents the server's SIGPIPE from killing the producer.
            // we actually don't want to die on SIGPIPE, we just want to catch the error and try again.
            if (send(sock, &msg, sizeof(LogMessage), MSG_NOSIGNAL) < 0) {
                printf(COLOR_RED "[Producer %d] Server is busy. Reconnecting...\n" COLOR_RESET, getpid());
                
                close(sock); // close the dead socket 
                sleep(1); // wait 1 second
                
                sock = -1;
                while (sock < 0) { // keep trying to reconnect with server
                    sock = connect_to_server(SERVER_IP, SERVER_PORT);
                    if (sock < 0) {
                        printf(COLOR_ORANGE "[Producer %d] Reconnection failed/refused. Retrying in 1s...\n" COLOR_RESET, getpid());
                        sleep(1); 
                    }
                }
                printf(COLOR_GREEN "[Producer %d] Reconnected successfully.\n" COLOR_RESET, getpid());
            } else {
                // succesfully sent the message, we can move on to the next one
                message_sent = 1;
                printf(COLOR_GRAY "[Producer %d] Message %d/%d sent (Data: %.2f)!\n" COLOR_RESET, getpid(), i+1, num_messages, msg.data);
            }
        }

        // add a random delay between messages, all but the last one
        if (i < num_messages - 1) {
            int delay_ms = 500 + (rand() % 1000); 
            usleep(delay_ms * 1000); 
        }
    }

    printf(COLOR_GRAY "[Producer %d] Finished sending data. Disconnecting.\n" COLOR_RESET, getpid());
    close(sock);
    return 0;
}