// chat_client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket"); exit(EXIT_FAILURE); }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect"); exit(EXIT_FAILURE);
    }

    char username[32];
    char buffer[BUFFER_SIZE];

    // Nhập username
    int n = recv(sockfd, buffer, sizeof(buffer)-1, 0);
    if (n > 0) { buffer[n] = '\0'; printf("%s", buffer); }
    fgets(username, sizeof(username), stdin);
    send(sockfd, username, strlen(username), 0);

    fd_set readfds;
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        FD_SET(STDIN_FILENO, &readfds);
        int maxfd = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;
        int ret = select(maxfd+1, &readfds, NULL, NULL, NULL);
        if (ret < 0) { perror("select"); break; }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            fgets(buffer, sizeof(buffer), stdin);
            send(sockfd, buffer, strlen(buffer), 0);
        }
        if (FD_ISSET(sockfd, &readfds)) {
            int n = recv(sockfd, buffer, sizeof(buffer)-1, 0);
            if (n <= 0) { printf("Disconnected from server.\n"); break; }
            buffer[n] = '\0';
            printf("%s", buffer);
        }
    }

    close(sockfd);
    return 0;
}