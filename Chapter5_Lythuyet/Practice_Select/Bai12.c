#include <stdio.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0); // Socket lắng nghe
    int client   = socket(AF_INET, SOCK_STREAM, 0); // Socket client (giả sử đã connect)

    if (listener < 0 || client < 0) {
        perror("Socket lỗi");
        return 1;
    }

    fd_set read_fds;
    int maxfd;

    FD_ZERO(&read_fds);
    FD_SET(listener, &read_fds);
    FD_SET(client, &read_fds);

    maxfd = (listener > client) ? listener : client;

    int result = select(maxfd + 1, &read_fds, NULL, NULL, NULL);

    if (result > 0) {
        if (FD_ISSET(listener, &read_fds)) {
            printf("Socket listener sẵn sàng nhận kết nối mới\n");
        }
        if (FD_ISSET(client, &read_fds)) {
            printf("Socket client sẵn sàng đọc dữ liệu\n");
        }
    }

    close(listener);
    close(client);
    return 0;
}