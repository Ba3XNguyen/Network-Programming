// client_improved.c
// Improved UDP client
// Features:
// - handshake with server
// - verify server with memcmp()
// - connect() on UDP after verify
// - multi-byte XOR encryption
// - timeout + retry
// - sequence number
// - can receive ACK and CHAT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define SERVER_PORT 9090
#define BUFFER_SIZE 1024
#define TIMEOUT_SEC 3
#define MAX_RETRIES 3

static const char *XOR_KEY = "labkey42";

void xor_cipher_buf(char *data, size_t len, const char *key) {
    size_t key_len = strlen(key);
    if (key_len == 0) return;
    for (size_t i = 0; i < len; i++) {
        data[i] ^= key[i % key_len];
    }
}

void trim_newline(char *s) {
    size_t len = strlen(s);
    if (len > 0 && s[len - 1] == '\n') s[len - 1] = '\0';
}

int wait_data(int fd, int timeout_sec) {
    fd_set rfds;
    struct timeval tv;

    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    tv.tv_sec = timeout_sec;
    tv.tv_usec = 0;

    return select(fd + 1, &rfds, NULL, NULL, &tv);
}

int send_encrypted_connected(int sockfd, const char *plain) {
    char out[BUFFER_SIZE];
    size_t len = strlen(plain);
    if (len >= sizeof(out)) len = sizeof(out) - 1;

    memcpy(out, plain, len);
    out[len] = '\0';
    xor_cipher_buf(out, len, XOR_KEY);

    return send(sockfd, out, len, 0);
}

int recv_decrypted_connected(int sockfd, char *out_plain, size_t out_size) {
    char in[BUFFER_SIZE];
    ssize_t n = recv(sockfd, in, sizeof(in) - 1, 0);
    if (n < 0) return -1;

    xor_cipher_buf(in, (size_t)n, XOR_KEY);
    in[n] = '\0';

    strncpy(out_plain, in, out_size - 1);
    out_plain[out_size - 1] = '\0';
    return (int)n;
}

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: %s <server_ip> [client_name]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *server_ip = argv[1];
    const char *client_name = (argc == 3) ? argv[2] : "anonymous";

    int sockfd;
    struct sockaddr_in server_addr, recv_addr;
    socklen_t recv_len;
    char buf[BUFFER_SIZE];
    int seq = 1;
    int client_id = 0;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        return EXIT_FAILURE;
    }

    // =========================
    // Phase 1: HELLO + verify server with memcmp()
    // =========================
    char hello_plain[BUFFER_SIZE];
    snprintf(hello_plain, sizeof(hello_plain), "HELLO|%d|%s", seq, client_name);

    {
        char enc[BUFFER_SIZE];
        size_t len = strlen(hello_plain);
        memcpy(enc, hello_plain, len);
        enc[len] = '\0';
        xor_cipher_buf(enc, len, XOR_KEY);

        if (sendto(sockfd, enc, len, 0,
                   (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("sendto");
            close(sockfd);
            return EXIT_FAILURE;
        }
    }

    printf("[CLIENT] HELLO sent. Waiting for WELCOME...\n");

    int ready = wait_data(sockfd, TIMEOUT_SEC);
    if (ready < 0) {
        perror("select");
        close(sockfd);
        return EXIT_FAILURE;
    }
    if (ready == 0) {
        printf("[CLIENT] Timeout waiting for server.\n");
        close(sockfd);
        return EXIT_FAILURE;
    }

    memset(&recv_addr, 0, sizeof(recv_addr));
    recv_len = sizeof(recv_addr);
    ssize_t n = recvfrom(sockfd, buf, sizeof(buf) - 1, 0,
                         (struct sockaddr *)&recv_addr, &recv_len);
    if (n < 0) {
        perror("recvfrom");
        close(sockfd);
        return EXIT_FAILURE;
    }

    // verify source server by memcmp()
    if (memcmp(&recv_addr.sin_addr, &server_addr.sin_addr, sizeof(struct in_addr)) != 0 ||
        memcmp(&recv_addr.sin_port, &server_addr.sin_port, sizeof(in_port_t)) != 0) {
        printf("[CLIENT] ERROR: packet not from expected server.\n");
        close(sockfd);
        return EXIT_FAILURE;
    }

    xor_cipher_buf(buf, (size_t)n, XOR_KEY);
    buf[n] = '\0';

    // Expected: WELCOME|client_id|seq|message
    char tmp[BUFFER_SIZE];
    strncpy(tmp, buf, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

    char *cmd = strtok(tmp, "|");
    char *id_str = strtok(NULL, "|");
    char *seq_str = strtok(NULL, "|");
    char *msg = strtok(NULL, "");

    if (!cmd || !id_str || !seq_str || strcmp(cmd, "WELCOME") != 0) {
        printf("[CLIENT] Invalid handshake response: %s\n", buf);
        close(sockfd);
        return EXIT_FAILURE;
    }

    client_id = atoi(id_str);
    int ack_seq = atoi(seq_str);
    if (ack_seq != seq) {
        printf("[CLIENT] Sequence mismatch during handshake.\n");
        close(sockfd);
        return EXIT_FAILURE;
    }

    printf("[CLIENT] Server verified with memcmp().\n");
    printf("[CLIENT] Handshake success. client_id=%d, message=%s\n",
           client_id, msg ? msg : "(none)");

    // =========================
    // Phase 2: connect() UDP after verification
    // =========================
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sockfd);
        return EXIT_FAILURE;
    }

    printf("[CLIENT] UDP connect() complete.\n");
    printf("[CLIENT] Type a message. '/quit' to exit.\n");

    seq++;

    while (1) {
        char input[BUFFER_SIZE];
        printf("You: ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        trim_newline(input);

        if (strcmp(input, "/quit") == 0) {
            char bye_plain[BUFFER_SIZE];
            snprintf(bye_plain, sizeof(bye_plain), "BYE|%d|bye", seq);

            int success = 0;
            for (int attempt = 1; attempt <= MAX_RETRIES; attempt++) {
                if (send_encrypted_connected(sockfd, bye_plain) < 0) {
                    perror("send");
                    break;
                }

                ready = wait_data(sockfd, TIMEOUT_SEC);
                if (ready < 0) {
                    perror("select");
                    break;
                }
                if (ready == 0) {
                    printf("[CLIENT] Timeout on BYE (attempt %d/%d)\n", attempt, MAX_RETRIES);
                    continue;
                }

                char resp[BUFFER_SIZE];
                if (recv_decrypted_connected(sockfd, resp, sizeof(resp)) < 0) {
                    perror("recv");
                    break;
                }

                printf("[CLIENT] Server: %s\n", resp);
                success = 1;
                break;
            }

            if (!success) {
                printf("[CLIENT] BYE not acknowledged, but client exits anyway.\n");
            }
            break;
        }

        char plain[BUFFER_SIZE];
        snprintf(plain, sizeof(plain), "MSG|%d|%s", seq, input);

        int delivered = 0;

        for (int attempt = 1; attempt <= MAX_RETRIES; attempt++) {
            if (send_encrypted_connected(sockfd, plain) < 0) {
                perror("send");
                break;
            }

            ready = wait_data(sockfd, TIMEOUT_SEC);
            if (ready < 0) {
                perror("select");
                break;
            }
            if (ready == 0) {
                printf("[CLIENT] Timeout waiting ACK (attempt %d/%d)\n", attempt, MAX_RETRIES);
                continue;
            }

            // One or more packets may arrive: ACK and/or CHAT
            // We read at least one packet after each send. If first packet is CHAT,
            // keep waiting for ACK until timeout expires again.
            int got_ack_for_this_seq = 0;

            while (1) {
                char resp[BUFFER_SIZE];
                if (recv_decrypted_connected(sockfd, resp, sizeof(resp)) < 0) {
                    perror("recv");
                    break;
                }

                // Parse response
                char copy[BUFFER_SIZE];
                strncpy(copy, resp, sizeof(copy) - 1);
                copy[sizeof(copy) - 1] = '\0';

                char *rcmd = strtok(copy, "|");
                char *rid = strtok(NULL, "|");
                char *rseq = strtok(NULL, "|");
                char *rrest = strtok(NULL, "");

                if (!rcmd) {
                    printf("[CLIENT] Malformed response: %s\n", resp);
                }
                else if (strcmp(rcmd, "ACK") == 0) {
                    int ack_id = rid ? atoi(rid) : 0;
                    int ack_seq2 = rseq ? atoi(rseq) : -1;

                    if (ack_id == client_id && ack_seq2 == seq) {
                        printf("[CLIENT] ACK: %s\n", rrest ? rrest : "");
                        got_ack_for_this_seq = 1;
                        delivered = 1;
                        break;
                    } else {
                        printf("[CLIENT] ACK for another seq/client ignored: %s\n", resp);
                    }
                }
                else if (strcmp(rcmd, "CHAT") == 0) {
                    printf("[CLIENT] Broadcast: %s\n", rrest ? rrest : "");
                }
                else if (strcmp(rcmd, "ERROR") == 0) {
                    printf("[CLIENT] Server error: %s\n", rrest ? rrest : resp);
                    got_ack_for_this_seq = 1;
                    delivered = 0;
                    break;
                }
                else {
                    printf("[CLIENT] Received: %s\n", resp);
                }

                // Wait a bit more for ACK in the same attempt
                ready = wait_data(sockfd, 1);
                if (ready <= 0) break;
            }

            if (got_ack_for_this_seq) break;
        }

        if (!delivered) {
            printf("[CLIENT] Message not confirmed after %d attempts.\n", MAX_RETRIES);
        }

        seq++;
    }

    close(sockfd);
    return 0;
}