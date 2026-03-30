#include <stdio.h>
#include <poll.h>

int main() {
    int sockfd1 = 3, sockfd2 = 4;

    struct pollfd fds[2];
    fds[0].fd = sockfd1;
    fds[0].events = POLLIN;
    fds[1].fd = sockfd2;
    fds[1].events = POLLIN;

    // poll() kiểm tra ngay lập tức, không chờ
    int ret = poll(fds, 2, 0);

    if (ret > 0) {
        for (int i = 0; i < 2; i++) {
            if (fds[i].revents & POLLIN) {
                printf("Socket %d có dữ liệu\n", fds[i].fd);
            }
        }
    } else if (ret == 0) {
        printf("Không có socket nào sẵn sàng ngay lập tức\n");
    } else {
        perror("poll error");
    }

    return 0;
}