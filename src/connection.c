#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <openssl/sha.h>
#include <openssl/buffer.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <sys/socket.h>

#include "../includes/connection.h"

#define GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

char* base64_encode(const unsigned char* input, int length) {
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *bio = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bio);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, input, length);
    BIO_flush(b64);
    BUF_MEM *buffer_ptr;
    BIO_get_mem_ptr(b64, &buffer_ptr);
    char* b64text = strndup(buffer_ptr->data, buffer_ptr->length);
    BIO_free_all(b64);
    return b64text;
}

void handle_websocket_connection(int client_fd,char request[BUF_SIZE]) {
    printf("Websocket Conneting...");
    char* key_start = strstr(request,"Sec-WebSocket-Key: ");
    if(!key_start) {
        close(client_fd);
        return;
    }

    key_start += strlen("Sec-WebSocket-Key: ");
    char websocket_key[256];
    int i = 0;
    while(key_start[i] != '\r' && key_start[i] != '\n' && i < 255) {
        websocket_key[i] = key_start[i];
        i++;
    }
    websocket_key[i] = '\0';

    //appending GUID and computing SHA1
    char accept_seed[512];
    snprintf(accept_seed,sizeof(accept_seed),"%s%s",websocket_key,GUID);

    unsigned char sha1_result[SHA_DIGEST_LENGTH];
    SHA1((unsigned char*)accept_seed,strlen(accept_seed),sha1_result);
    printf("Sha1 Result sha1_result: %s",sha1_result);

    //encoding base64
    char* accept_key = base64_encode(sha1_result,SHA_DIGEST_LENGTH);
    printf("Base64 Encoded accept_key: %s",accept_key);

    char response[1024];
    snprintf(response,sizeof(response),
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n\r\n",
        accept_key);

    write(client_fd,response,strlen(response));
    free(accept_key);

    printf("✅ WebSocket handshake completed.\n");

    
}

void* handle_client(void *arg) {
    printf("Client thread started\n");
    
    int client_fd = *(int *)arg;
    free(arg);

    char request[BUF_SIZE];
    int received = recv(client_fd,request,BUF_SIZE,0);
    if(received <= 0) {
        close(client_fd);
        return NULL;
    }
    request[received] = '\0';
    printf("Client request:\n%s\n", request);

    if(strstr(request, "Connection: Upgrade") && strstr(request,"Upgrade: websocket")) {
        printf("Websocket Upgrade requested.");
        handle_websocket_connection(client_fd,request);
        return NULL;
    }

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
        
        write(client_fd,header,strlen(header));
        write(client_fd,html_content,size);
        free(html_content);

        close(client_fd);
        return NULL;
}