// client.c
// gcc -O2 -Wall -Wextra -o client client.c

#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUF_SIZE 4096

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in srv = {0};
    srv.sin_family = AF_INET;
    srv.sin_port = htons(5000);
    inet_pton(AF_INET, "127.0.0.1", &srv.sin_addr);

    connect(fd, (struct sockaddr*)&srv, sizeof(srv));

    char buf[BUF_SIZE];

    while (1) {
        int n = recv(fd, buf, sizeof(buf)-1, 0);
        if (n <= 0) break;

        buf[n] = '\0';
        printf("%s", buf);

        if (strstr(buf, "Chon")) {
            fgets(buf, sizeof(buf), stdin);
            send(fd, buf, strlen(buf), 0);
        }
    }

    close(fd);
    return 0;
}