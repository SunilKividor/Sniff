#ifndef SERVER_H
#define SERVER_H

#include "../includes/websockets.h"

typedef struct {
   ws_clients_t* ws_clients;
   int* client_socket_fd; 
} ws_handler_args_t;

typedef struct
{
    char *filePath;
    int *file_changed;
    pthread_cond_t *cond;
    pthread_mutex_t *mutex;
} file_watcher_args_t;

typedef struct
{
    ws_clients_t *clients;
    file_watcher_args_t *file_watcher_args;
} ws_monitor_args_t;

#endif