#ifndef NETWORK_H
#define NETWORK_H

int setup_server_socket(int port, int maxconnections);
int connect_to_server(const char* ip, int port);

#endif