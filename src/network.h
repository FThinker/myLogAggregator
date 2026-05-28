#ifndef NETWORK_H
#define NETWORK_H


// ----------------------------------------------------------------------------------------- //
/**
 * @brief Sets up a socket on the specified port and starts listening.
 * @param port The port number to bind the server socket to.
 * @param maxconnections The maximum number of pending connections to allow.
 * @return The file descriptor of the server socket on success, -1 on failure.
 */

int setup_server_socket(int port, int maxconnections);


// ----------------------------------------------------------------------------------------- //

/**
 * @brief Connects to a server at the specified IP address and port.
 * @param ip The IP address of the target server.
 * @param port The port number of the target server.
 * @return The connected socket on success, -1 on failure.
 */
int connect_to_server(const char* ip, int port);


// ----------------------------------------------------------------------------------------- //


#endif