#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <netdb.h>
#include <stdbool.h>

#include "../includes/socket.h"
#include "../includes/connection.h"
#include "../includes/websockets.h"
#include "../includes/file_watcher.h"

typedef struct {
    ws_clients_t* clients;
    bool* file_changed;
    pthread_mutex_t* mutex;
} ws_monitor_args_t;

int main(int argc,char *argv[]) {
    char* filePathName = "./src/index.html";
    int initFileWatcher = init_file_watcher(filePathName);
    if(initFileWatcher == -1) {
        perror("[FILE WATCHER] Error Initializing File Watcher");
        return -1;
    }

    if(start_file_watcher(filePathName) < 0) {
        perror("[FILE WATCHER] Error starting File Watcher thread");
        return -1;
    }

    int server_socket_fd = startServer();

    while(1) {
        int *client_socket_fd = malloc(sizeof(int));
        *client_socket_fd = accept(server_socket_fd,NULL,NULL);
        if(*client_socket_fd == -1) {
            fprintf(stderr,"Could not accpet\n");
            free(client_socket_fd);
            continue;
        }
   
        pthread_t tid;
        pthread_create(&tid,NULL,handle_client,client_socket_fd);
        pthread_detach(tid);
    }

    return 0;
}