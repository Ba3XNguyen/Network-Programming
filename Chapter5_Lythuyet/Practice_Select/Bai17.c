#include <stdio.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket lỗi");
        return 1;
    }

    fd_set exceptfds;
    FD_ZERO(&exceptfds);
    FD_SET(sockfd, &exceptfds);

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    int result = select(sockfd + 1, NULL, NULL, &exceptfds, &timeout);

    if (result > 0) {
        if (FD_ISSET(sockfd, &exceptfds)) {
            printf("Socket có ngoại lệ\n");
        }
    } else if (result == 0) {
        printf("Timeout hết hạn\n");
    } else {
        perror("select lỗi");
    }

    close(sockfd);
    return 0;
}