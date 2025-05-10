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


#define BOLD "\033[1m"
#define COLOR_GREEN "\033[32m"
#define COLOR_WHITE "\033[37m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_CYAN "\033[36m"
#define COLOR_RESET "\033[0m"

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

    printf("%s%s┃                                               ┃%s\n", BOLD, COLOR_GREEN, COLOR_RESET);
    printf("%s%s┃  %sS E R V E R   S T A R T E D   %s               ┃%s\n", BOLD, COLOR_GREEN, COLOR_WHITE, COLOR_GREEN, COLOR_RESET);
    printf("%s%s┃                                               ┃%s\n", BOLD, COLOR_GREEN, COLOR_RESET);
    printf("%s%s┃  %sPORT:%s %-37d  ┃%s\n", BOLD, COLOR_GREEN, COLOR_YELLOW, COLOR_CYAN, 8080, COLOR_RESET);
    printf("%s%s┃                                               ┃%s\n", BOLD, COLOR_GREEN, COLOR_RESET);
    printf("%s%s┃  %sHOT RELOAD:%s %-31s  ┃%s\n", BOLD, COLOR_GREEN, COLOR_YELLOW, COLOR_CYAN, "ENABLED", COLOR_RESET);
    printf("%s%s┃                                               ┃%s\n", BOLD, COLOR_GREEN, COLOR_RESET);
    printf("%s%s┃  %s[FILE WATCHER]:%s %-28s ┃%s\n", BOLD, COLOR_GREEN, COLOR_YELLOW, COLOR_CYAN, "RUNNING", COLOR_RESET);
    printf("%s%s┃                                               ┃%s\n", BOLD, COLOR_GREEN, COLOR_RESET);

    while(1) {
        int *client_socket_fd = malloc(sizeof(int));
        *client_socket_fd = accept(server_socket_fd,NULL,NULL);
        if(*client_socket_fd == -1) {
            fprintf(stderr,"Could not accpet\n");
            free(client_socket_fd);
            continue;
        }
        printf("Accepted client_fd: %d\n", *client_socket_fd);
        pthread_t tid;
        pthread_create(&tid,NULL,handle_client,client_socket_fd);
        pthread_detach(tid);
    }

    return 0;
}