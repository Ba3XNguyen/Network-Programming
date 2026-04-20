#include <stdio.h>
#include <poll.h>

int main() {
    int sockfd = 3;

    struct pollfd fds[1];
    fds[0].fd = sockfd;
    fds[0].events = POLLOUT; // Kiểm tra khi socket sẵn sàng ghi

    int ret = poll(fds, 1, 5000); // timeout 5 giây
    if (ret > 0) {
        if (fds[0].revents & POLLOUT) {
            printf("Socket %d sẵn sàng ghi\n", fds[0].fd);
        }
    } else if (ret == 0) {
        printf("Timeout, socket không sẵn sàng ghi\n");
    } else {
        perror("poll error");
    }

    return 0;
}