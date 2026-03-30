#include <stdio.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0); // Tạo socket
    if (sockfd < 0) {
        perror("Socket lỗi");
        return 1;
    }

    fd_set read_fds;
    struct timeval timeout;

    while (1) {
        // Cần khởi tạo lại tập fd_set trước mỗi lần select()
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);

        // Timeout 5 giây
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        int result = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);

        if (result > 0) {
            printf("Socket sẵn sàng\n");
        } else if (result == 0) {
            printf("Timeout\n");
        } else {
            perror("select lỗi");
        }

        // Lý do: select() sẽ thay đổi fd_set, chỉ giữ lại FD sẵn sàng
        // Nếu không làm mới, lần gọi select() tiếp theo sẽ mất các FD ban đầu
    }

    close(sockfd);
    return 0;
}