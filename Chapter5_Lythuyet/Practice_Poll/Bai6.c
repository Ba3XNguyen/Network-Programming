#include <stdio.h>
#include <poll.h>

int main() {
    struct pollfd fds[1];

    fds[0].fd = -1;      // FD không hợp lệ
    fds[0].events = POLLIN;

    int ret = poll(fds, 1, 1000); // timeout 1 giây
    if (ret > 0) {
        if (fds[0].revents & POLLNVAL) {
            printf("FD không hợp lệ\n");
        }
    } else if (ret == 0) {
        printf("Timeout, không có sự kiện\n");
    } else {
        perror("poll error");
    }

    return 0;
}