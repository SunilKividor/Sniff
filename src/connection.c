#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "../includes/connection.h"

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