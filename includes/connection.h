#ifndef CONNECTION_H
#define CONNECTION_H

#include "../includes/websockets.h"

#define BUF_SIZE 1028

void send_websocket_frame(ws_clients_t* clients, const char *message);
char *base64_encode(const unsigned char *input, int length);
int process_websocket_handshake(int client_fd, char *buffer);
void add_client(int client_fd,ws_clients_t* ws_clients);
char* get_message();
void broadcast_message(ws_clients_t* clients,char* message);
void* handle_websocket_connection(void *arg);
void* handle_http_client(void *arg);

#endif