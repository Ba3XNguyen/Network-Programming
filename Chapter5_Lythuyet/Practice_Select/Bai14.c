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

    fd_set read_fds;

    for (int t = 1; t <= 5; t++) { // Ví dụ timeout thay đổi 1-5 giây
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);

        struct timeval timeout;
        timeout.tv_sec = t; // timeout động
        timeout.tv_usec = 0;

        int result = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);

        if (result > 0) {
            printf("Socket sẵn sàng (timeout = %d giây)\n", t);
        } else if (result == 0) {
            printf("Timeout %d giây hết hạn\n", t);
        } else {
            perror("select lỗi");
        }
    }

    close(sockfd);
    return 0;
}