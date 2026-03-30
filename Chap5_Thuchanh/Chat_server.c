// chat_server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/poll.h>

#include "utils.h"

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

static volatile sig_atomic_t stop_server = 0;
void handle_sigint(int sig) {
    (void)sig;
    stop_server = 1;
}

int main() {
    signal(SIGINT, handle_sigint);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); exit(EXIT_FAILURE); }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 5) < 0) { perror("listen"); exit(EXIT_FAILURE); }

    printf("Chat server started on port %d\n", PORT);

    struct pollfd fds[MAX_CLIENTS + 1]; // 0 là server_fd
    int client_sockets[MAX_CLIENTS] = {0};
    char usernames[MAX_CLIENTS][32] = {{0}};

    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    while (!stop_server) {
        int nfds = 1;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] > 0) {
                fds[nfds].fd = client_sockets[i];
                fds[nfds].events = POLLIN;
                nfds++;
            }
        }

        int ret = poll(fds, nfds, 500);
        if (ret < 0) { if (errno == EINTR) continue; perror("poll"); break; }
        if (ret == 0) continue;

        // Mới client kết nối
        if (fds[0].revents & POLLIN) {
            int new_fd = accept(server_fd, NULL, NULL);
            if (new_fd >= 0) {
                char name[32];
                send(new_fd, "Enter your username: ", 21, 0);
                int n = recv(new_fd, name, sizeof(name)-1, 0);
                if (n > 0) { name[n-1] = '\0'; } else { strncpy(name, "Anonymous", 32); }

                int idx = add_client(client_sockets, usernames, new_fd, name, MAX_CLIENTS);
                if (idx >= 0) {
                    char msg[BUFFER_SIZE];
                    snprintf(msg, sizeof(msg), "%s has joined the chat.\n", name);
                    printf("%s", msg);
                    broadcast_message(new_fd, client_sockets, usernames, MAX_CLIENTS, msg);
                } else {
                    send(new_fd, "Server full\n", 12, 0);
                    close(new_fd);
                }
            }
        }

        // Xử lý client gửi tin nhắn
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int fd = client_sockets[i];
            if (fd > 0 && FD_ISSET(fd, fds, nfds)) {
                char buf[BUFFER_SIZE];
                int n = recv(fd, buf, sizeof(buf)-1, 0);
                if (n <= 0) {
                    // Client disconnect
                    char msg[BUFFER_SIZE];
                    snprintf(msg, sizeof(msg), "%s has left the chat.\n", usernames[i]);
                    printf("%s", msg);
                    broadcast_message(fd, client_sockets, usernames, MAX_CLIENTS, msg);
                    remove_client(client_sockets, usernames, i);
                    close(fd);
                } else {
                    buf[n] = '\0';
                    char msg[BUFFER_SIZE];
                    snprintf(msg, sizeof(msg), "%s: %s", usernames[i], buf);
                    printf("%s", msg);
                    broadcast_message(fd, client_sockets, usernames, MAX_CLIENTS, msg);
                }
            }
        }
    }

    // Close all
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (client_sockets[i] > 0) close(client_sockets[i]);
    close(server_fd);
    printf("Server stopped.\n");
    return 0;
}