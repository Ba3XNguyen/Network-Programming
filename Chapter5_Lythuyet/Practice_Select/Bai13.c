#include <stdio.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0); // Socket client
    if (sockfd < 0) {
        perror("Socket lỗi");
        return 1;
    }

    fd_set read_fds;
    int maxfd;

    FD_ZERO(&read_fds);
    FD_SET(0, &read_fds);      // stdin
    FD_SET(sockfd, &read_fds); // socket

    maxfd = (sockfd > 0) ? sockfd : 0;

    int result = select(maxfd + 1, &read_fds, NULL, NULL, NULL);

    if (result > 0) {
        if (FD_ISSET(0, &read_fds)) {
            printf("Dữ liệu có sẵn trên stdin\n");
        }
        if (FD_ISSET(sockfd, &read_fds)) {
            printf("Dữ liệu có sẵn trên socket\n");
        }
    }

    close(sockfd);
    return 0;
}