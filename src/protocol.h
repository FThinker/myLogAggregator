#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>


// ----------------------------------------------------------------------------------------- //


// network constants
#define SERVER_PORT 10467
#define SERVER_IP "127.0.0.1" // localhost
#define MAX_BACKLOG 250 // maximum simultaneous connections


// ----------------------------------------------------------------------------------------- //

// the "skeleton" format for a log
typedef struct {
    int sender_id;
    double data;
} LogMessage;


// ----------------------------------------------------------------------------------------- //

#endif