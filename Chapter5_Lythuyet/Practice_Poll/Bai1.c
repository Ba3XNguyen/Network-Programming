#include <stdio.h>
#include <poll.h>

int main() {
    struct pollfd fds[1];

    fds[0].fd = 0;      // stdin
    fds[0].events = POLLIN; // Theo dõi sự kiện đọc

    printf("Đã khởi tạo pollfd cho stdin\n");
    return 0;
}