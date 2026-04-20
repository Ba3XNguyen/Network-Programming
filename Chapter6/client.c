// client.c
// UDP client with XOR encryption, server verification by memcmp(), timeout using select()
// It first uses sendto/recvfrom to verify server, then uses connect() to simplify later communication.

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
#define XOR_KEY 0x5A
#define TIMEOUT_SEC 5

void xor_cipher(char *data, char key) {
    for (int i = 0; data[i] != '\0'; i++) {
        data[i] ^= key;
    }
}

void trim_newline(char *s) {
    size_t len = strlen(s);
    if (len > 0 && s[len - 1] == '\n') {
        s[len - 1] = '\0';
    }
}

int wait_for_data(int fd, int timeout_sec) {
    fd_set readfds;
    struct timeval tv;

    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    tv.tv_sec = timeout_sec;
    tv.tv_usec = 0;

    return select(fd + 1, &readfds, NULL, NULL, &tv);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <server_ip>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int sockfd;
    struct sockaddr_in server_addr, recv_addr;
    socklen_t recv_len;
    char send_buf[BUFFER_SIZE];
    char recv_buf[BUFFER_SIZE];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        return EXIT_FAILURE;
    }

    // =========================
    // Phase 1: verify server using recvfrom() + memcmp()
    // =========================
    strcpy(send_buf, "HELLO");
    xor_cipher(send_buf, XOR_KEY);

    if (sendto(sockfd, send_buf, strlen(send_buf), 0,
               (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("sendto");
        close(sockfd);
        return EXIT_FAILURE;
    }

    printf("[CLIENT] HELLO sent to server, waiting for response...\n");

    int ready = wait_for_data(sockfd, TIMEOUT_SEC);
    if (ready < 0) {
        perror("select");
        close(sockfd);
        return EXIT_FAILURE;
    } else if (ready == 0) {
        printf("[CLIENT] Timeout: server did not respond.\n");
        close(sockfd);
        return EXIT_FAILURE;
    }

    memset(recv_buf, 0, sizeof(recv_buf));
    memset(&recv_addr, 0, sizeof(recv_addr));
    recv_len = sizeof(recv_addr);

    ssize_t n = recvfrom(sockfd, recv_buf, BUFFER_SIZE - 1, 0,
                         (struct sockaddr *)&recv_addr, &recv_len);
    if (n < 0) {
        perror("recvfrom");
        close(sockfd);
        return EXIT_FAILURE;
    }
    recv_buf[n] = '\0';

    // Verify server address using memcmp()
    if (memcmp(&recv_addr.sin_addr, &server_addr.sin_addr, sizeof(struct in_addr)) != 0 ||
        memcmp(&recv_addr.sin_port, &server_addr.sin_port, sizeof(in_port_t)) != 0) {
        printf("[CLIENT] ERROR: message not from expected server. Discarded.\n");
        close(sockfd);
        return EXIT_FAILURE;
    }

    printf("[CLIENT] Server verified successfully using memcmp().\n");

    xor_cipher(recv_buf, XOR_KEY);
    printf("[CLIENT] Server says: %s\n", recv_buf);

    // =========================
    // Phase 2: connect() UDP socket to simplify communication
    // =========================
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sockfd);
        return EXIT_FAILURE;
    }

    printf("[CLIENT] UDP socket connected to server. Type messages (type 'exit' to quit).\n");

    while (1) {
        printf("You: ");
        fflush(stdout);

        if (fgets(send_buf, sizeof(send_buf), stdin) == NULL) {
            break;
        }

        trim_newline(send_buf);

        if (strcmp(send_buf, "exit") == 0) {
            printf("[CLIENT] Exiting.\n");
            break;
        }

        xor_cipher(send_buf, XOR_KEY);

        // After connect(), we can use send()/recv() instead of sendto()/recvfrom()
        if (send(sockfd, send_buf, strlen(send_buf), 0) < 0) {
            perror("send");
            break;
        }

        ready = wait_for_data(sockfd, TIMEOUT_SEC);
        if (ready < 0) {
            perror("select");
            break;
        } else if (ready == 0) {
            printf("[CLIENT] Timeout: no reply from server.\n");
            continue;
        }

        memset(recv_buf, 0, sizeof(recv_buf));
        n = recv(sockfd, recv_buf, BUFFER_SIZE - 1, 0);
        if (n < 0) {
            perror("recv");
            break;
        }

        recv_buf[n] = '\0';
        xor_cipher(recv_buf, XOR_KEY);
        printf("Server: %s\n", recv_buf);
    }

    close(sockfd);
    return 0;
}