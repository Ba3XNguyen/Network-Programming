#include <sys/select.h>
#include <stdio.h>

int main() {
    fd_set read_fds;
    int sockfd1 = 5, sockfd2 = 6;
    int maxfd = (sockfd1 > sockfd2) ? sockfd1 : sockfd2;

    FD_ZERO(&read_fds);
    FD_SET(sockfd1, &read_fds);
    FD_SET(sockfd2, &read_fds);

    // Giả sử select đã trả về
    select(maxfd + 1, &read_fds, NULL, NULL, NULL);

    for (int i = 0; i <= maxfd; i++) {
        if (FD_ISSET(i, &read_fds)) {
            printf("Socket %d sẵn sàng đọc\n", i);
        }
    }

    return 0;
}