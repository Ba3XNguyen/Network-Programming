#include <stdio.h>
#include <poll.h>
#include <unistd.h> // close()

int main() {
    int sockfd = 3; // Giả sử đây là socket client đã kết nối

    struct pollfd fds[1];
    fds[0].fd = sockfd;
    fds[0].events = POLLIN; // Theo dõi sự kiện đọc

    int ret = poll(fds, 1, 5000); // timeout 5 giây
    if (ret > 0) {
        if (fds[0].revents & POLLHUP) {
            printf("Client đóng kết nối\n");
            close(sockfd); // Đóng socket sau khi client ngắt
        } else if (fds[0].revents & POLLIN) {
            printf("Có dữ liệu từ client\n");
        }
    } else if (ret == 0) {
        printf("Timeout, không có sự kiện\n");
    } else {
        perror("poll error");
    }

    return 0;
}