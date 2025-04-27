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

#include "../includes/socket.h"

#define BUF_SIZE 1028

#define MAX_CLIENTS 30

typedef struct {
    int client_sockets[MAX_CLIENTS];
    int count;
    pthread_mutex_t mutex;
} ws_clients_t;

void* handle_client(void *arg) {
    printf("Client thread started\n");
    
    int client_fd = *(int *)arg;
    free(arg);

    char request[BUF_SIZE];
    read(client_fd,request,BUF_SIZE);

    int fileFD = open("/home/ubuntu/simple-tcp-server/src/index.html",O_RDONLY);
    if(fileFD == -1) {
        perror("open index.html");
        close(client_fd);
        pthread_exit(NULL);
    }
    off_t size = lseek(fileFD,0,SEEK_END);
    lseek(fileFD,0,SEEK_SET);

    char *html_content = malloc(size +1);
    read(fileFD,html_content,size);
    html_content[size] = 0;
    close(fileFD);

    char header[256];
    sprintf(header,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %ld\r\n"
        "\r\n", size);
        
        printf("HTML content: %s\n", html_content);
        write(client_fd,header,strlen(header));
        write(client_fd,html_content,size);
        free(html_content);

        close(client_fd);
        return NULL;
}

int main(int argc,char *argv[]) {
    // if(argc != 2) {
    //     fprintf(stderr,"Usage: %s port\n",argv[1]);
    //     exit(EXIT_FAILURE);
    // }

    // if(argv[1] < 0) {
    //     perror("INVALID PORT NUMBER. TRY A DIFFERENT PORT");
    //     return -1;
    // }

    // setPort(PORT);
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