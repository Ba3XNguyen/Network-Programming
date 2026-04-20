// server.c
// UDP multi-client chat server with simple XOR encryption and timeout using select()

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <errno.h>

#define SERVER_PORT 9090
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100
#define XOR_KEY 0x5A
#define TIMEOUT_SEC 10

typedef struct {
    struct sockaddr_in addr;
    int active;
    int id;
} ClientInfo;

// XOR encryption/decryption (same function for both)
void xor_cipher(char *data, char key) {
    for (int i = 0; data[i] != '\0'; i++) {
        data[i] ^= key;
    }
}

// Compare two client addresses (IP + port)
int same_client(const struct sockaddr_in *a, const struct sockaddr_in *b) {
    return (a->sin_family == b->sin_family &&
            a->sin_port == b->sin_port &&
            a->sin_addr.s_addr == b->sin_addr.s_addr);
}

// Find existing client in list
int find_client(ClientInfo clients[], const struct sockaddr_in *addr) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && same_client(&clients[i].addr, addr)) {
            return i;
        }
    }
    return -1;
}

// Add new client to list
int add_client(ClientInfo clients[], const struct sockaddr_in *addr) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            clients[i].addr = *addr;
            clients[i].active = 1;
            clients[i].id = i + 1;
            return i;
        }
    }
    return -1;
}

void print_client_addr(const struct sockaddr_in *addr) {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr->sin_addr), ip, sizeof(ip));
    printf("%s:%d", ip, ntohs(addr->sin_port));
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    char buffer[BUFFER_SIZE];
    ClientInfo clients[MAX_CLIENTS] = {0};

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;   // listen on all interfaces
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("UDP server is listening on port %d...\n", SERVER_PORT);

    while (1) {
        fd_set readfds;
        struct timeval tv;

        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        tv.tv_sec = TIMEOUT_SEC;
        tv.tv_usec = 0;

        int ready = select(sockfd + 1, &readfds, NULL, NULL, &tv);

        if (ready < 0) {
            perror("select");
            break;
        } else if (ready == 0) {
            printf("[SERVER] Timeout: no message received in %d seconds.\n", TIMEOUT_SEC);
            continue;
        }

        if (FD_ISSET(sockfd, &readfds)) {
            memset(buffer, 0, sizeof(buffer));
            client_len = sizeof(client_addr);

            ssize_t n = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0,
                                 (struct sockaddr *)&client_addr, &client_len);
            if (n < 0) {
                perror("recvfrom");
                continue;
            }

            buffer[n] = '\0';

            // Decrypt incoming message
            xor_cipher(buffer, XOR_KEY);

            int idx = find_client(clients, &client_addr);
            if (idx == -1) {
                idx = add_client(clients, &client_addr);
                if (idx == -1) {
                    fprintf(stderr, "[SERVER] Client list full.\n");
                    continue;
                }
                printf("[SERVER] New client #%d registered: ", clients[idx].id);
                print_client_addr(&client_addr);
                printf("\n");
            }

            printf("[SERVER] Received from client #%d (", clients[idx].id);
            print_client_addr(&client_addr);
            printf("): %s\n", buffer);

            // Prepare different response for each client
            char reply[BUFFER_SIZE];
            if (strcmp(buffer, "HELLO") == 0) {
                snprintf(reply, sizeof(reply),
                         "WELCOME client #%d - secure UDP chat started",
                         clients[idx].id);
            } else {
                snprintf(reply, sizeof(reply),
                         "ACK client #%d: server received -> %s",
                         clients[idx].id, buffer);
            }

            // Encrypt before sending
            xor_cipher(reply, XOR_KEY);

            if (sendto(sockfd, reply, strlen(reply), 0,
                       (struct sockaddr *)&client_addr, client_len) < 0) {
                perror("sendto");
                continue;
            }

            printf("[SERVER] Encrypted reply sent to client #%d\n", clients[idx].id);
        }
    }

    close(sockfd);
    return 0;
}