#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include "protocol.h"
#include "network.h"

#define COLOR_RED   "\033[0;31m"
#define COLOR_GRAY "\033[0;90m"
#define COLOR_ORANGE "\033[0;33m"
#define COLOR_GREEN "\033[0;32m"
#define COLOR_RESET "\033[0m"

#define MAX_RETRIES 5 // max number of connection retries before giving up


// ----------------------------------------------------------------------------------------- //


void print_usage(const char* prog_name) {
    printf("\n");
    printf("======================= Producer =======================\n");
    printf(COLOR_GRAY);
    printf("Usage: %s [-d] [-r <value>]\n", prog_name);
    printf("Options:\n");
    printf("  -d, --debug   Enable debug mode\n");
    printf("  -r, --random  Send a random number of messages (default: 1-5)\n");
    printf(COLOR_RESET);
    printf("========================================================\n");   
    printf("\n");
}


// ----------------------------------------------------------------------------------------- //


int main(int argc, char *argv[]) {
    int opt;
    int option_index = 0;

    bool debug_mode = false;
    bool random_mode = false;
    int max_random_messages = 5; // default max number of messages in random mode

    // map of long to short options
    static struct option long_options[] = {
        {"debug",  no_argument, 0,  'd' },
        {"random", optional_argument, 0,  'r' },
        {0,0,0,0} // terminator
    };

    // we only care about "d" and "r" options. "d" has no arguments, "r" has an optional argument
    while ((opt = getopt_long(argc, argv, "dr::", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'd':
                debug_mode = true;
                break;
            case 'r':
                random_mode = true;
                if (optarg != NULL) { // if user writes -r10 or --random=10, optarg will point to "10"
                    max_random_messages = atoi(optarg);
                } 
                else if (optind < argc && argv[optind][0] != '-') { // if user writes -r 10 WITH A SPACE
                    max_random_messages = atoi(argv[optind]);
                    optind++; // optind is incremented to skip the number in the next iteration of getopt_long
                }
                if (max_random_messages <= 0) { // if the user provided an invalid number, we reset to default
                    max_random_messages = 5; 
                }

                break;
            case '?':
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // generate a random seed based on current time and PID
    srand(time(NULL) ^ getpid());

    if(debug_mode) printf(COLOR_GRAY "[Producer %d] Starting producer...\n" COLOR_RESET, getpid());

    int sock = -1;
    int retries = 0;

    // keep trying to connect until successful (or hit max retries cap)
    while (sock < 0 && retries < MAX_RETRIES) { 
        sock = connect_to_server(SERVER_IP, SERVER_PORT);
        if (sock < 0) {
            retries++;
            if(debug_mode) printf(COLOR_ORANGE "[Producer %d] Connection failed. Retrying in 1s (Attempt %d/%d)...\n" COLOR_RESET, getpid(), retries, MAX_RETRIES);
            if (retries < MAX_RETRIES) sleep(1);
        }
    }

    // if out of retries, gracefully DIE
    if (sock < 0) {
        if(debug_mode) printf(COLOR_RED "[Producer %d] Max connection retries reached. Exiting.\n" COLOR_RESET, getpid());
        return 1;
    }

    if(debug_mode) printf(COLOR_GREEN "[Producer %d] Succesfully connected to coordinator.\n" COLOR_RESET, getpid());

    // number of messages sent is controlled by flags
    int num_messages = 1; // default behaviour
    if (random_mode) {
        num_messages = (rand() % max_random_messages) + 1; // default 1-5 messages, controlled by -r <val> flag.
        if(debug_mode) printf(COLOR_GRAY "[Producer %d] Random mode enabled. Will send %d messages.\n" COLOR_RESET, getpid(), num_messages);
    }

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
                if(debug_mode) printf(COLOR_RED "[Producer %d] Server is busy. Reconnecting...\n" COLOR_RESET, getpid());
                
                close(sock); // close the dead socket 
                sleep(1); // wait 1 second
                
                sock = -1;
                int reconn_retries = 0;

                while (sock < 0 && reconn_retries < MAX_RETRIES) { // again, keep trying to reconnect until successful or max retries hit
                    sock = connect_to_server(SERVER_IP, SERVER_PORT);
                    if (sock < 0) {
                        reconn_retries++;
                        if(debug_mode) printf(COLOR_ORANGE "[Producer %d] Reconnection failed. Retrying in 1s (Attempt %d/%d)...\n" COLOR_RESET, getpid(), reconn_retries, MAX_RETRIES);
                        if (reconn_retries < MAX_RETRIES) sleep(1); 
                    }
                }
                
                // if out of retries, again, die...
                if (sock < 0) {
                    if(debug_mode) printf(COLOR_RED "[Producer %d] Max reconnection retries reached. Exiting.\n" COLOR_RESET, getpid());
                    return 1;
                }

                if(debug_mode) printf(COLOR_GREEN "[Producer %d] Reconnected successfully.\n" COLOR_RESET, getpid());
            } else {
                // succesfully sent the message, we can move on to the next one
                message_sent = 1;
                if(debug_mode) printf(COLOR_GRAY "[Producer %d] Message %d/%d sent (Data: %.2f)!\n" COLOR_RESET, getpid(), i+1, num_messages, msg.data);
            }
        }

        // add a random delay between messages, all but the last one, and only if messages > 1
        if (i < num_messages - 1) {
            int delay_ms = 500 + (rand() % 1000); 
            usleep(delay_ms * 1000); 
        }
    }
    // print final message regardless of debug mode
    printf(COLOR_GRAY "[Producer %d] Finished sending data. Disconnecting.\n" COLOR_RESET, getpid());
    close(sock);
    exit(EXIT_SUCCESS);
}


// ----------------------------------------------------------------------------------------- //