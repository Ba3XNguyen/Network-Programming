#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main() {
    fd_set read_fds;
    int sockfd;

    // Tạo socket TCP
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Lỗi tạo socket");
        exit(EXIT_FAILURE);
    }

    // Khởi tạo tập FD rỗng và thêm socket vào
    FD_ZERO(&read_fds);
    FD_SET(sockfd, &read_fds);

    // Bài 2: Kiểm tra socket có trong tập FD không
    if (FD_ISSET(sockfd, &read_fds)) {
        printf("Socket %d có trong fd_set và sẵn sàng để đọc.\n", sockfd);
    } else {
        printf("Socket %d không có trong fd_set.\n", sockfd);
    }

    // Đóng socket
    close(sockfd);
    return 0;
}