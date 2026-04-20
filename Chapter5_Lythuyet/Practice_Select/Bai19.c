#include <stdio.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

int main() {
    int sockets[100]; // ví dụ 100 socket
    fd_set read_fds;
    FD_ZERO(&read_fds);

    for (int i = 0; i < 100; i++) {
        sockets[i] = socket(AF_INET, SOCK_STREAM, 0);
        FD_SET(sockets[i], &read_fds);
    }

    int maxfd = sockets[99];
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    clock_t start = clock();
    int ready = select(maxfd + 1, &read_fds, NULL, NULL, &timeout);
    clock_t end = clock();

    double elapsed_ms = (double)(end - start) / CLOCKS_PER_SEC * 1000;
    printf("Select kiểm tra %d socket mất %.2f ms, socket sẵn sàng: %d\n", 100, elapsed_ms, ready);

    for (int i = 0; i < 100; i++) close(sockets[i]);
    return 0;
}