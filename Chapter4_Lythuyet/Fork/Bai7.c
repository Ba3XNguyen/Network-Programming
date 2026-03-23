#include <stdio.h>
#include <unistd.h>

int listener = 3;   // giả sử socket server
int client_fd = 4;  // socket client

int main() {
    if (fork() == 0) {
        // ---- CHILD ----
        close(listener); // đóng socket lắng nghe

        printf("Child xu ly client\n");

        close(client_fd);
    } else {
        // ---- PARENT ----
        close(client_fd); // cha không xử lý client
    }

    return 0;
}