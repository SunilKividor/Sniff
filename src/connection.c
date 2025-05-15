#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
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
#include "../includes/server.h"
#include "../includes/websockets.h"

#define GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

void send_websocket_frame(ws_clients_t *clients, const char *message)
{
    size_t msg_len = strlen(message);
    uint8_t *frame = malloc(10 + msg_len);

    size_t pos = 0;
    frame[pos++] = 0x81;

    if (msg_len <= 125)
    {
        frame[pos++] = msg_len;
    }
    else if (msg_len <= 65535)
    {
        frame[pos++] = 126;
        frame[pos++] = (msg_len >> 8) & 0xFF;
        frame[pos++] = msg_len & 0xFF;
    }
    else
    {
        frame[pos++] = 127;
        for (int i = 7; i >= 0; --i)
        {
            frame[pos++] = (msg_len >> (8 * i)) & 0xFF;
        }
    }

    memcpy(&frame[pos], message, msg_len);
    pos += msg_len;

    int i = 0;
    if (clients->count > 0)
    {
        while (i < clients->count)
        {
            int client_fd = clients->client_sockets[i];
            send(client_fd, frame, pos, 0);
            i++;
        }
    }
}

char *base64_encode(const unsigned char *input, int length)
{
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *bio = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bio);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, input, length);
    BIO_flush(b64);
    BUF_MEM *buffer_ptr;
    BIO_get_mem_ptr(b64, &buffer_ptr);
    char *b64text = strndup(buffer_ptr->data, buffer_ptr->length);
    BIO_free_all(b64);
    return b64text;
}

int process_websocket_handshake(int client_fd, char *buffer)
{
    // process handshake
    printf("Inside process_websocket_handshake\n");
    char *key_start = strstr(buffer, "Sec-WebSocket-Key: ");
    if (!key_start)
    {
        close(client_fd);
        return 0;
    }

    key_start += strlen("Sec-WebSocket-Key: ");
    char websocket_key[256];
    int i = 0;
    while (key_start[i] != '\r' && key_start[i] != '\n' && i < 255)
    {
        websocket_key[i] = key_start[i];
        i++;
    }
    websocket_key[i] = '\0';

    // appending GUID and computing SHA1
    char accept_seed[512];
    snprintf(accept_seed, sizeof(accept_seed), "%s%s", websocket_key, GUID);

    unsigned char sha1_result[SHA_DIGEST_LENGTH];
    SHA1((unsigned char *)accept_seed, strlen(accept_seed), sha1_result);

    // base64
    char *accept_key = base64_encode(sha1_result, SHA_DIGEST_LENGTH);

    char response[1024];
    snprintf(response, sizeof(response),
             "HTTP/1.1 101 Switching Protocols\r\n"
             "Upgrade: websocket\r\n"
             "Connection: Upgrade\r\n"
             "Sec-WebSocket-Accept: %s\r\n\r\n",
             accept_key);

    send(client_fd, response, strlen(response), 0);
    free(accept_key);

    printf("WebSocket handshake completed.\n");
    return 1;
}

void add_client(int client_fd, ws_clients_t *ws_clients)
{
    pthread_mutex_lock(&ws_clients->mutex);

    if (ws_clients->count < MAX_WS_CLIENTS)
    {
        ws_clients->client_sockets[ws_clients->count++] = client_fd;
        printf("Client added at index %d\n", ws_clients->count - 1);
    }

    pthread_mutex_unlock(&ws_clients->mutex);
}

char *get_message()
{
    int fileFD = open("/home/ubuntu/simple-tcp-server/src/index.html", O_RDONLY);
    if (fileFD == -1)
    {
        perror("open index.html");
        return NULL;
    }
    off_t size = lseek(fileFD, 0, SEEK_END);
    lseek(fileFD, 0, SEEK_SET);

    char *html_content = malloc(size + 1);
    read(fileFD, html_content, size);
    html_content[size] = 0;
    close(fileFD);

    return html_content;
}

void broadcast_message(ws_clients_t *clients, char *message)
{

    if (clients->count > 0)
    {
        send_websocket_frame(clients, message);
    }
    else
    {
        printf("[WEBSOCKETS: NO ACTIVE CLIENTS]\n");
    }

    free(message);
}

void *handle_websocket_connection(void *arg)
{
    printf("Inside handle_websocket_connection\n");
    ws_handler_args_t ws_handler_arg = *(ws_handler_args_t *)arg;

    int client_fd = *(ws_handler_arg.client_socket_fd);
    ws_clients_t *ws_clients = ws_handler_arg.ws_clients;

    free(arg);

    char buffer[BUF_SIZE] = {0};
    int bytes_read = recv(client_fd, buffer, BUF_SIZE, 0);

    printf("Bytes read: %d\n", bytes_read);

    if (process_websocket_handshake(client_fd, buffer))
    {
        add_client(client_fd, ws_clients);
        char *message = get_message();
        broadcast_message(ws_clients, message);
    }
    else
    {
        perror("error processing websocket handshake");
        close(client_fd);
        return NULL;
    }
}

void *handle_http_client(void *arg)
{

    int client_fd = *(int *)arg;
    free(arg);

    printf("Client fd: %d\n", client_fd);

    int fileFD = open("/home/ubuntu/simple-tcp-server/src/index.html", O_RDONLY);
    if (fileFD == -1)
    {
        perror("open index.html");
        close(client_fd);
        return NULL;
    }
    off_t size = lseek(fileFD, 0, SEEK_END);
    lseek(fileFD, 0, SEEK_SET);

    char *html_content = malloc(size + 1);
    read(fileFD, html_content, size);
    html_content[size] = 0;
    close(fileFD);

    char header[256];
    sprintf(header,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: %ld\r\n"
            "\r\n",
            size);

    ssize_t sent = send(client_fd, header, strlen(header), 0);
    if (sent == -1)
        perror("send header");

    sent = send(client_fd, html_content, size, 0);
    if (sent == -1)
        perror("send body");
    free(html_content);

    close(client_fd);
    return NULL;
}