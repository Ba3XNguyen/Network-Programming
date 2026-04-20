#include <stdio.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>    // cho close()
#include <sys/socket.h>
#include <netinet/in.h>

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0); // Tạo socket
    if (sockfd < 0) {
        perror("Socket lỗi");
        return 1;
    }

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(sockfd, &read_fds);

    // Thiết lập timeout 5 giây
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    int maxfd = sockfd; // Sử dụng để truyền vào select()

    int result = select(maxfd + 1, &read_fds, NULL, NULL, &timeout);

    if (result > 0) {
        printf("Có ít nhất một socket sẵn sàng\n");
    } else if (result == 0) {
        printf("Timeout: không có socket nào sẵn sàng\n");
    } else {
        perror("select lỗi");
    }

    close(sockfd);
    return 0;
}