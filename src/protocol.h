#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

// network constants
#define SERVER_PORT 10467
#define SERVER_IP "127.0.0.1"
#define MAX_BACKLOG 10 // maximum simultaneous connections

typedef struct {
    int sender_id;
    double data;
} LogMessage;

#endif