#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main() {
    fd_set read_fds;             // Khai báo tập file descriptor
    int sockfd;

    // Tạo socket TCP
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Lỗi tạo socket");
        exit(EXIT_FAILURE);
    }

    // Bước 1: Khởi tạo tập FD rỗng
    FD_ZERO(&read_fds);

    // Bước 2: Thêm sockfd vào tập FD
    FD_SET(sockfd, &read_fds);

    // Kiểm tra xem sockfd đã được thêm vào tập chưa
    if (FD_ISSET(sockfd, &read_fds)) {
        printf("Socket %d đã được thêm vào fd_set.\n", sockfd);
    } else {
        printf("Socket %d chưa được thêm vào fd_set.\n", sockfd);
    }

    // Đóng socket khi xong
    close(sockfd);
    return 0;
}