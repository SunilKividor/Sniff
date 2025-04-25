#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#define BUF_SIZE 1028
#define MAX_LISTEN_BACKLOG 10

void* handle_client(void *arg) {
    printf("Client thread started\n");
    
    int client_fd = *(int *)arg;
    free(arg);

    char request[BUF_SIZE];
    read(client_fd,request,BUF_SIZE);

    int fileFD = open("index.html",O_RDONLY);
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
        "Content-Length: %lld\r\n"
        "\r\n", size);
    
        write(client_fd,header,strlen(header));
        write(client_fd,html_content,size);
        free(html_content);
    
        return NULL;
}

int main(int argc,char *argv[]) {
    if(argc != 2) {
        fprintf(stderr,"Usage: %s port\n",argv[2]);
        exit(EXIT_FAILURE);
    }

    struct addrinfo hints;
    struct addrinfo *addrs,*addr_iter;

    int server_socket_fd;
    int so_reuseaddr;
 
    memset(&hints,0,sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    int getaddrinfo_error = getaddrinfo(NULL,argv[1],&hints,&addrs);
    if(getaddrinfo_error != 0) {
        fprintf(stderr,"getaddrinfo: %s \n",gai_strerror(getaddrinfo_error));
        exit(EXIT_FAILURE);
    }

    for(addr_iter = addrs;addr_iter != NULL;addr_iter = addr_iter->ai_next) {
        server_socket_fd =  socket(addr_iter->ai_family,addr_iter->ai_socktype,addr_iter->ai_protocol);

        if(server_socket_fd == -1) {
            continue;
        }

        so_reuseaddr = 1;
        setsockopt(server_socket_fd,SOL_SOCKET,SO_REUSEADDR,&so_reuseaddr,sizeof(so_reuseaddr));

        if(bind(server_socket_fd,addr_iter->ai_addr,addr_iter->ai_addrlen) == 0){
            break;
        }

        close(server_socket_fd);
    }

    freeaddrinfo(addrs);

    if(addr_iter == NULL) {
        fprintf(stderr,"Could not bind\n");
        exit(EXIT_FAILURE);
    }

    if(listen(server_socket_fd,MAX_LISTEN_BACKLOG) != 0) {
        fprintf(stderr,"error listening to socket %d\n",server_socket_fd);
        exit(EXIT_FAILURE);   
    }

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