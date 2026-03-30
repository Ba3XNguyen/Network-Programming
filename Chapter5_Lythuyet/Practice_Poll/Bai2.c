#include <stdio.h>
#include <poll.h>

int main() {
    struct pollfd fds[1];
    fds[0].fd = 0;         // stdin
    fds[0].events = POLLIN;

    int ret = poll(fds, 1, 5000); // 5 giây timeout

    if (ret == -1) {
        perror("poll error");
    } else if (ret == 0) {
        printf("Timeout, không có dữ liệu\n");
    } else {
        if (fds[0].revents & POLLIN) {
            printf("Có dữ liệu từ stdin\n");
        }
    }

    return 0;
}