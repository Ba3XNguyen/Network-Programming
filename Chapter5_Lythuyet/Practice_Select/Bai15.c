#include <stdio.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main() {
    int sockfd1 = socket(AF_INET, SOCK_STREAM, 0);
    int sockfd2 = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd1 < 0 || sockfd2 < 0) {
        perror("Socket lỗi");
        return 1;
    }

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(sockfd1, &read_fds);
    FD_SET(sockfd2, &read_fds);

    int maxfd = (sockfd1 > sockfd2) ? sockfd1 : sockfd2;

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    int ready = select(maxfd + 1, &read_fds, NULL, NULL, &timeout);
    printf("%d socket sẵn sàng\n", ready);

    close(sockfd1);
    close(sockfd2);
    return 0;
}