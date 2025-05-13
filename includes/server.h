#ifndef SERVER_H
#define SERVER_H

#include "../includes/websockets.h"

typedef struct {
   ws_clients_t* ws_clients;
   int* client_socket_fd; 
} ws_handler_args_t;

#endif