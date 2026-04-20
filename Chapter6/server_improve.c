// server_improved.c
// Improved UDP multi-client chat server
// Features:
// - multiple clients
// - handshake (HELLO -> WELCOME)
// - multi-byte XOR encryption
// - sequence number / ACK
// - timeout with select()
// - inactive client cleanup
// - broadcast chat messages to other clients

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <time.h>

#define SERVER_PORT 9090
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100
#define TIMEOUT_SEC 2
#define CLIENT_INACTIVE_SEC 30

static const char *XOR_KEY = "labkey42";

typedef struct {
    struct sockaddr_in addr;
    int active;
    int verified;
    int client_id;
    int last_seq_received;
    time_t last_seen;
    char name[32];
} ClientInfo;

void xor_cipher_buf(char *data, size_t len, const char *key) {
    size_t key_len = strlen(key);
    if (key_len == 0) return;
    for (size_t i = 0; i < len; i++) {
        data[i] ^= key[i % key_len];
    }
}

int same_addr(const struct sockaddr_in *a, const struct sockaddr_in *b) {
    return a->sin_family == b->sin_family &&
           a->sin_port == b->sin_port &&
           a->sin_addr.s_addr == b->sin_addr.s_addr;
}

void print_addr(const struct sockaddr_in *addr) {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr->sin_addr), ip, sizeof(ip));
    printf("%s:%d", ip, ntohs(addr->sin_port));
}

int find_client(ClientInfo clients[], const struct sockaddr_in *addr) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && same_addr(&clients[i].addr, addr)) {
            return i;
        }
    }
    return -1;
}

int add_client(ClientInfo clients[], const struct sockaddr_in *addr, int assigned_id) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            memset(&clients[i], 0, sizeof(clients[i]));
            clients[i].active = 1;
            clients[i].verified = 0;
            clients[i].client_id = assigned_id;
            clients[i].last_seq_received = -1;
            clients[i].last_seen = time(NULL);
            clients[i].addr = *addr;
            snprintf(clients[i].name, sizeof(clients[i].name), "client%d", assigned_id);
            return i;
        }
    }
    return -1;
}

void cleanup_inactive_clients(ClientInfo clients[]) {
    time_t now = time(NULL);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && (now - clients[i].last_seen > CLIENT_INACTIVE_SEC)) {
            printf("[SERVER] Removing inactive client #%d (", clients[i].client_id);
            print_addr(&clients[i].addr);
            printf(")\n");
            memset(&clients[i], 0, sizeof(clients[i]));
        }
    }
}

void send_encrypted_message(int sockfd, const struct sockaddr_in *addr, const char *plain_text) {
    char out[BUFFER_SIZE];
    size_t len = strlen(plain_text);
    if (len >= sizeof(out)) len = sizeof(out) - 1;

    memcpy(out, plain_text, len);
    out[len] = '\0';

    xor_cipher_buf(out, len, XOR_KEY);

    if (sendto(sockfd, out, len, 0, (const struct sockaddr *)addr, sizeof(*addr)) < 0) {
        perror("sendto");
    }
}

void broadcast_chat(int sockfd, ClientInfo clients[], int sender_index, int seq, const char *msg_body) {
    char plain[BUFFER_SIZE];
    snprintf(plain, sizeof(plain),
             "CHAT|0|%d|from=%d|%s",
             seq,
             clients[sender_index].client_id,
             msg_body);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) continue;
        if (!clients[i].verified) continue;
        if (i == sender_index) continue;

        send_encrypted_message(sockfd, &clients[i].addr, plain);
    }
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr, src_addr;
    socklen_t src_len = sizeof(src_addr);
    char buf[BUFFER_SIZE];
    ClientInfo clients[MAX_CLIENTS];
    int next_client_id = 1;

    memset(clients, 0, sizeof(clients));

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(sockfd);
        return EXIT_FAILURE;
    }

    printf("[SERVER] Listening on UDP port %d\n", SERVER_PORT);

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
        }

        cleanup_inactive_clients(clients);

        if (ready == 0) {
            // periodic timeout only for maintenance / logging
            continue;
        }

        if (!FD_ISSET(sockfd, &readfds)) {
            continue;
        }

        memset(buf, 0, sizeof(buf));
        memset(&src_addr, 0, sizeof(src_addr));
        src_len = sizeof(src_addr);

        ssize_t n = recvfrom(sockfd, buf, sizeof(buf) - 1, 0,
                             (struct sockaddr *)&src_addr, &src_len);
        if (n < 0) {
            perror("recvfrom");
            continue;
        }

        xor_cipher_buf(buf, (size_t)n, XOR_KEY);
        buf[n] = '\0';

        int idx = find_client(clients, &src_addr);

        // Expected format:
        // HELLO|seq|name
        // MSG|seq|text
        // BYE|seq|reason
        //
        // Responses:
        // WELCOME|client_id|seq|message
        // ACK|client_id|seq|message
        // CHAT|0|seq|from=id|text
        // ERROR|client_id|seq|message

        char work[BUFFER_SIZE];
        strncpy(work, buf, sizeof(work) - 1);
        work[sizeof(work) - 1] = '\0';

        char *cmd = strtok(work, "|");
        char *seq_str = strtok(NULL, "|");
        char *rest = strtok(NULL, "");

        if (!cmd || !seq_str) {
            send_encrypted_message(sockfd, &src_addr, "ERROR|0|0|Malformed packet");
            continue;
        }

        int seq = atoi(seq_str);

        if (strcmp(cmd, "HELLO") == 0) {
            if (idx == -1) {
                idx = add_client(clients, &src_addr, next_client_id++);
                if (idx == -1) {
                    send_encrypted_message(sockfd, &src_addr, "ERROR|0|0|Server full");
                    continue;
                }
                printf("[SERVER] New client #%d registered from ", clients[idx].client_id);
                print_addr(&src_addr);
                printf("\n");
            }

            clients[idx].verified = 1;
            clients[idx].last_seen = time(NULL);
            clients[idx].last_seq_received = seq;

            if (rest && strlen(rest) > 0) {
                strncpy(clients[idx].name, rest, sizeof(clients[idx].name) - 1);
                clients[idx].name[sizeof(clients[idx].name) - 1] = '\0';
            }

            char reply[BUFFER_SIZE];
            snprintf(reply, sizeof(reply),
                     "WELCOME|%d|%d|Hello %s",
                     clients[idx].client_id, seq, clients[idx].name);
            send_encrypted_message(sockfd, &src_addr, reply);

            printf("[SERVER] Handshake completed with client #%d (%s)\n",
                   clients[idx].client_id, clients[idx].name);
            continue;
        }

        if (idx == -1 || !clients[idx].verified) {
            send_encrypted_message(sockfd, &src_addr, "ERROR|0|0|Handshake required");
            continue;
        }

        clients[idx].last_seen = time(NULL);

        if (seq <= clients[idx].last_seq_received) {
            char dup_ack[BUFFER_SIZE];
            snprintf(dup_ack, sizeof(dup_ack),
                     "ACK|%d|%d|Duplicate or old packet ignored",
                     clients[idx].client_id, seq);
            send_encrypted_message(sockfd, &src_addr, dup_ack);
            continue;
        }

        clients[idx].last_seq_received = seq;

        if (strcmp(cmd, "MSG") == 0) {
            char text[BUFFER_SIZE];
            snprintf(text, sizeof(text), "%s", rest ? rest : "");

            printf("[SERVER] MSG from #%d (%s): %s\n",
                   clients[idx].client_id, clients[idx].name, text);

            char ack[BUFFER_SIZE];
            snprintf(ack, sizeof(ack),
                     "ACK|%d|%d|Message delivered",
                     clients[idx].client_id, seq);
            send_encrypted_message(sockfd, &src_addr, ack);

            broadcast_chat(sockfd, clients, idx, seq, text);
        }
        else if (strcmp(cmd, "BYE") == 0) {
            char ack[BUFFER_SIZE];
            snprintf(ack, sizeof(ack),
                     "ACK|%d|%d|Goodbye",
                     clients[idx].client_id, seq);
            send_encrypted_message(sockfd, &src_addr, ack);

            printf("[SERVER] Client #%d disconnected\n", clients[idx].client_id);
            memset(&clients[idx], 0, sizeof(clients[idx]));
        }
        else {
            char err[BUFFER_SIZE];
            snprintf(err, sizeof(err),
                     "ERROR|%d|%d|Unknown command",
                     clients[idx].client_id, seq);
            send_encrypted_message(sockfd, &src_addr, err);
        }
    }

    close(sockfd);
    return 0;
}