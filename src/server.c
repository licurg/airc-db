#include "../includes/server.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

int
start_server(
    void *handler
) {
    int socket_status, client_socket, *new_socket;
    struct sockaddr_in server, client;
    socket_status = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_status == -1) {
        printf("\033[0;31mSocket isn't started.\033[0m\n");
        return -1;
    }
    else printf("Socket is started.\n");
    
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(HOST);
    server.sin_port = htons(PORT);

    if (bind(socket_status, (struct sockaddr*) &server, sizeof(server)) < 0) {
        perror("\033[0;31mBind failed. Error\033[0m");
        return -1;
    }
    printf("Bind done.\n");

    listen(socket_status, 3);
    printf("Server is listening on %s:%d\n", HOST, PORT);

    int c = sizeof(struct sockaddr_in);
    printf("Waiting for connections...\n");
    while (client_socket = accept(socket_status, (struct sockaddr*) &client, (socklen_t*) &c))
    {
        printf("\n\033[0;32mConnection accepted.\033[0m\n");

        pthread_t s_thread;
        new_socket = (int*)malloc(sizeof(int));
        *new_socket = client_socket;
        if (pthread_create(&s_thread, NULL, handler, (void*)new_socket) < 0) {
            perror("\033[0;31mThread create failed. Error\033[0m");
            return -1;
        }

        printf("Handle assigned.\n");
    }
    if (client_socket < 0) {
        perror("\033[0;31mAccept failed. Error\033[0m");
        return -1;
    }
}