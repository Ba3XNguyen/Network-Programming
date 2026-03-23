// server.c
#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#define PORT 5000
#define BACKLOG 10
#define BUF_SIZE 4096
#define MAX_Q 10

typedef struct {
    char question[256];
    char options[4][100];
    int correct;
} Question;

Question quiz[MAX_Q] = {
    {"2+2=?", {"1","2","3","4"}, 3},
    {"5*3=?", {"15","10","8","20"}, 0},
    {"10/2=?", {"2","5","10","20"}, 1},
    {"3^2=?", {"6","9","3","12"}, 1},
    {"7-4=?", {"1","2","3","4"}, 2},
    {"6+1=?", {"5","6","7","8"}, 2},
    {"8/4=?", {"1","2","3","4"}, 1},
    {"9-3=?", {"5","6","7","8"}, 1},
    {"2*6=?", {"10","11","12","13"}, 2},
    {"1+9=?", {"9","10","11","12"}, 1}
};

static ssize_t send_all(int fd, const void *buf, size_t len) {
    const char *p = buf;
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, p + sent, len - sent, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        sent += n;
    }
    return sent;
}

static ssize_t recv_line(int fd, char *buf, size_t cap) {
    size_t used = 0;
    while (used + 1 < cap) {
        char c;
        ssize_t n = recv(fd, &c, 1, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) break;
        buf[used++] = c;
        if (c == '\n') break;
    }
    buf[used] = '\0';
    return used;
}

void shuffle(int *a) {
    for (int i = 0; i < 4; i++) {
        int r = rand() % 4;
        int tmp = a[i]; a[i] = a[r]; a[r] = tmp;
    }
}

void handle_client(int fd) {
    char buf[BUF_SIZE];
    int score = 0;

    for (int i = 0; i < MAX_Q; i++) {
        int idx[4] = {0,1,2,3};
        shuffle(idx);

        int correct_new = 0;
        for (int j = 0; j < 4; j++)
            if (idx[j] == quiz[i].correct) correct_new = j;

        snprintf(buf, sizeof(buf), "\nCau %d: %s\n", i+1, quiz[i].question);
        send_all(fd, buf, strlen(buf));

        for (int j = 0; j < 4; j++) {
            snprintf(buf, sizeof(buf), "%d. %s\n", j+1, quiz[i].options[idx[j]]);
            send_all(fd, buf, strlen(buf));
        }

        send_all(fd, "Chon (1-4): \n", 14);

        if (recv_line(fd, buf, sizeof(buf)) <= 0) break;

        int ans = atoi(buf) - 1;

        if (ans == correct_new) {
            score++;
            send_all(fd, "Dung!\n", 6);
        } else {
            send_all(fd, "Sai!\n", 5);
        }
    }

    snprintf(buf, sizeof(buf), "\nDiem: %d/%d\n", score, MAX_Q);
    send_all(fd, buf, strlen(buf));

    close(fd);
}

int main() {
    signal(SIGCHLD, SIG_IGN); // chống zombie
    srand(time(NULL));

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in srv = {0};
    srv.sin_family = AF_INET;
    srv.sin_port = htons(PORT);
    srv.sin_addr.s_addr = INADDR_ANY;

    bind(listen_fd, (struct sockaddr*)&srv, sizeof(srv));
    listen(listen_fd, BACKLOG);

    printf("Server dang chay port %d...\n", PORT);

    while (1) {
        int client_fd = accept(listen_fd, NULL, NULL);
        if (client_fd < 0) continue;

        pid_t pid = fork();

        if (pid == 0) {
            close(listen_fd);
            handle_client(client_fd);
            exit(0);
        } else {
            close(client_fd);
        }
    }
}