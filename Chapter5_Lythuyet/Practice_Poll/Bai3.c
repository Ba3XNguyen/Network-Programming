#include <stdio.h>
#include <poll.h>

int main() {
    int sockfd1 = 3; // giả sử socket 1
    int sockfd2 = 4; // giả sử socket 2

    struct pollfd fds[2];

    fds[0].fd = sockfd1;
    fds[0].events = POLLIN;

    fds[1].fd = sockfd2;
    fds[1].events = POLLIN;

    int ret = poll(fds, 2, 10000); // 10 giây timeout
    if (ret == -1) {
        perror("poll error");
    } else if (ret == 0) {
        printf("Timeout, không có dữ liệu trên socket\n");
    }

    return 0;
}