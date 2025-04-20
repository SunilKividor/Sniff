#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#define BUF_SIZE 1028

int main(int argc,char *argv[]) {
    if(argc != 3) {
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

    int getaddrinfo_error = getaddrinfo(NULL,argv[2],&hints,&addrs);
    if(getaddrinfo_error != 0) {
        fprintf(stderr,"getaddrinfo: %s \n",gai_strerror(getaddrinfo_error));
        exit(EXIT_FAILURE);
    }

    for(addr_iter = addrs;addr_iter != NULL;addr_iter = addrs->ai_next) {
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

    printf("server socket fd : %d",server_socket_fd);

    return 0;
}