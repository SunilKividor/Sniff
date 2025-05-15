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
#include "../includes/server.h"

#define BOLD "\033[1m"
#define COLOR_GREEN "\033[32m"
#define COLOR_WHITE "\033[37m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_CYAN "\033[36m"
#define COLOR_RESET "\033[0m"

int is_websocket_connection(char *buffer)
{
    printf("Checking connection type...\n");
    if (strstr(buffer, "Connection: Upgrade") && strstr(buffer, "Upgrade: websocket"))
    {
        return 1;
    }

    return 0;
}

void *ws_monitor(void *arg)
{
    ws_monitor_args_t *ws_monitor_args = (ws_monitor_args_t *)arg;

    file_watcher_args_t *file_watcher_args = ws_monitor_args->file_watcher_args;
    ws_clients_t *ws_clients = ws_monitor_args->clients;

    while (1)
    {
        pthread_mutex_lock(file_watcher_args->mutex);

        while (!(*file_watcher_args->file_changed))
        {
            pthread_cond_wait(file_watcher_args->cond, file_watcher_args->mutex);
        }

        *file_watcher_args->file_changed = 0;
        pthread_mutex_unlock(file_watcher_args->mutex);

        printf("file changed.Update ws clients\n");
        char* message = get_message();
        broadcast_message(ws_clients,message);
    }

    return NULL;
}

pthread_t start_monitoring_file(void *args)
{
    pthread_t tid;
    pthread_create(&tid, NULL, ws_monitor, args);
    pthread_detach(tid);
    return tid;
}

int main(int argc, char *argv[])
{

    char *filePath = "./src/index.html";

    int* fileChanged = malloc(sizeof(int));
    *fileChanged = 0;

    // initializing websocket args
    ws_clients_t *ws_clients = malloc(sizeof(ws_clients_t)); // creating ws clients for global use
    memset(ws_clients->client_sockets, 0, sizeof(ws_clients->client_sockets));
    ws_clients->count = 0;
    pthread_mutex_init(&ws_clients->mutex, NULL);

    // initializing file watcher args
    file_watcher_args_t *file_watcher_args = malloc(sizeof(file_watcher_args_t));
    file_watcher_args->filePath = filePath;
    // file_watcher_args->file_changed = malloc(sizeof(int));
    file_watcher_args->file_changed = fileChanged;
    file_watcher_args->cond = malloc(sizeof(pthread_cond_t));
    pthread_cond_init(file_watcher_args->cond, NULL);
    file_watcher_args->mutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(file_watcher_args->mutex, NULL);

    //initializingf monitor args
    ws_monitor_args_t* ws_monitor_args = malloc(sizeof(ws_monitor_args_t));
    ws_monitor_args->clients = ws_clients;
    ws_monitor_args->file_watcher_args = file_watcher_args;

    // initializing file watcher
    int initFileWatcher = init_file_watcher(filePath);
    if (initFileWatcher == -1)
    {
        perror("[FILE WATCHER] Error Initializing File Watcher");
        return -1;
    }

    // starting file notifier
    if (start_file_watcher(file_watcher_args) < 0)
    {
        perror("[FILE WATCHER] Error starting File Watcher thread");
        return -1;
    }

    if(start_monitoring_file(ws_monitor_args) < 0) {
        perror("[MONITOR] Error starting monitor thread");
        return -1;
    }

    // starting server
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

    while (1)
    {
        int *client_socket_fd = malloc(sizeof(int));
        *client_socket_fd = accept(server_socket_fd, NULL, NULL);
        if (*client_socket_fd == -1)
        {
            fprintf(stderr, "Could not accpet\n");
            free(client_socket_fd);
            continue;
        }

        char request[BUF_SIZE] = {0};
        int bytes_read = recv(*client_socket_fd, request, BUF_SIZE, MSG_PEEK);
        if (bytes_read > 0)
        {
            if (is_websocket_connection(request))
            {
                ws_handler_args_t *ws_client_handler_args = malloc(sizeof(ws_handler_args_t));
                ws_client_handler_args->client_socket_fd = client_socket_fd;
                ws_client_handler_args->ws_clients = ws_clients;
                printf("Websocket Upgrade requested.\n");
                pthread_t tid;
                pthread_create(&tid, NULL, handle_websocket_connection, ws_client_handler_args);
                pthread_detach(tid);
            }
            else
            {
                printf("HTTP requested.\n");
                pthread_t tid;
                pthread_create(&tid, NULL, handle_http_client, client_socket_fd);
                pthread_detach(tid);
            }
        }
        else
        {
            close(*client_socket_fd);
        }
    }

    return 0;
}