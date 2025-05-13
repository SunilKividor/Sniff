#ifndef WEBSOCKETS_H
#define WEBSOCKETS_H

#include <pthread.h>
#define MAX_WS_CLIENTS 30

typedef struct {
    int client_sockets[MAX_WS_CLIENTS];
    int count;
    pthread_mutex_t mutex;
} ws_clients_t;

#endif