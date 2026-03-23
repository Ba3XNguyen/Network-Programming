#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>

int main() {
    int server_fd; // giả sử đã tạo socket + bind + listen

    for (int i = 0; i < 3; i++) {
        fork(); // tạo nhiều tiến trình
    }

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        printf("Process %d nhan client\n", getpid());

        // xử lý client
        close(client_fd);
    }

    return 0;
}