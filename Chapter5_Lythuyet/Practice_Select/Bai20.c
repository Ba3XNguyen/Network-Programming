#include <stdio.h>
#include <sys/select.h>
#include <unistd.h>

int my_select(int maxfd, fd_set *readfds, struct timeval *timeout) {
    return select(maxfd + 1, readfds, NULL, NULL, timeout);
}

int main() {
    fd_set read_fds;
    FD_ZERO(&read_fds);

    int sockfd = 0; // ví dụ stdin
    FD_SET(sockfd, &read_fds);

    struct timeval timeout = {5, 0};

    int ready = my_select(sockfd, &read_fds, &timeout);

    if (ready > 0) {
        printf("Có FD sẵn sàng\n");
    } else if (ready == 0) {
        printf("Timeout\n");
    } else {
        perror("select lỗi");
    }

    return 0;
}