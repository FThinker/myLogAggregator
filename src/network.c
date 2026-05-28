#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "network.h"

#define COLOR_RED   "\033[0;31m"
#define COLOR_RESET "\033[0m"

// Create server socket, apply SO_REUSEADDR and bind+listen
int setup_server_socket(int port, int maxconnections) {
    int server_fd  = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd == -1){
        perror(COLOR_RED "[NETWORK] Error while creating socket" COLOR_RESET);
        exit(EXIT_FAILURE);
    }
    
    // rapid reuse of IP:PORT via REUSEADDR
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror(COLOR_RED "[NETWORK] Error in setsockopt" COLOR_RESET);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror(COLOR_RED "[NETWORK] Error while attempting bind between socket %d and address %s" COLOR_RESET, server_fd, inet_ntoa(address.sin_addr));
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror(COLOR_RED "[NETWORK] Error while attempting listen on socket %d" COLOR_RESET, server_fd);
        exit(EXIT_FAILURE);
    }

    return server_fd;
}

int connect_to_server(const char* ip, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror(COLOR_RED "[NETWORK] Error creating socket for client" COLOR_RESET);
        return -1;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        perror(COLOR_RED "[NETWORK] Address not valid or not supported" COLOR_RESET);
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror(COLOR_RED "[NETWORK] Error connecting to server" COLOR_RESET);
        close(sock);
        return -1;
    }
    return sock;
}