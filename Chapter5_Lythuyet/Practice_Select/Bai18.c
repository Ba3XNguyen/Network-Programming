#include <stdio.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main() {
    int sockets[FD_SETSIZE]; // FD_SETSIZE thường là 1024
    fd_set read_fds;
    FD_ZERO(&read_fds);

    // Khởi tạo và thêm tối đa FD_SETSIZE socket vào fd_set
    for (int i = 0; i < FD_SETSIZE; i++) {
        sockets[i] = socket(AF_INET, SOCK_STREAM, 0);
        if (sockets[i] < 0) {
            perror("Socket lỗi");
            return 1;
        }
        FD_SET(sockets[i], &read_fds);
    }

    int maxfd = sockets[FD_SETSIZE - 1]; // socket lớn nhất
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    int ready = select(maxfd + 1, &read_fds, NULL, NULL, &timeout);
    printf("%d socket sẵn sàng trong tối đa FD_SETSIZE\n", ready);

    for (int i = 0; i < FD_SETSIZE; i++) {
        close(sockets[i]);
    }

    return 0;
}