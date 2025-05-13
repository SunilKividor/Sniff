#ifndef CONNECTION_H
#define CONNECTION_H

#define BUF_SIZE 1028
void* handle_http_client(void *arg);
void* handle_websocket_connection(void *arg);

#endif