#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>

#include "../includes/socket.h"

// void setPort(int port)
// {
//     PORT = port;
// }

int startServer()
{
    struct addrinfo hints;
    struct addrinfo *addrs, *addr_iter;
    int server_socket_fd;
    int so_reuseaddr;
    int retries = 0;
    int bind_success = 0;
    int retry_delay = 1;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    int getaddrinfo_error = getaddrinfo(NULL, "8080", &hints, &addrs);
    if (getaddrinfo_error != 0)
    {
        fprintf(stderr, "getaddrinfo: %s \n", gai_strerror(getaddrinfo_error));
        return -1;
    }

    while (retries < MAX_RETRIES)
    {
        for (addr_iter = addrs; addr_iter != NULL; addr_iter = addr_iter->ai_next)
        {
            server_socket_fd = socket(addr_iter->ai_family, addr_iter->ai_socktype, addr_iter->ai_protocol);

            if (server_socket_fd == -1)
            {
                continue;
            }

            so_reuseaddr = 1;
            setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr, sizeof(so_reuseaddr));

            if (bind(server_socket_fd, addr_iter->ai_addr, addr_iter->ai_addrlen) == 0)
            {
                bind_success = 1;
                fprintf(stderr, "Bind succeded in (%d/%d) tries\n", retries+1, MAX_RETRIES);
                break;
            }

            close(server_socket_fd);
        }

        if(bind_success) {
            break;
        }
        
        retries++;
        fprintf(stderr, "Bind failed, retrying (%d/%d)...\n", retries+1, MAX_RETRIES);
        sleep(retry_delay);
    }

    freeaddrinfo(addrs);

    if (!bind_success)
    {
        fprintf(stderr, "Could not bind\n");
        return -1;
    }

    if (listen(server_socket_fd, MAX_LISTEN_BACKLOG) != 0)
    {
        fprintf(stderr, "error listening to socket %d\n", server_socket_fd);
        return -1;
    }

    return server_socket_fd;
}