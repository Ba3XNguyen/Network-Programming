#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[1024] = {0};

    // 1. Tạo socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // 2. Cấu hình địa chỉ
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 3. Bind
    bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));

    // 4. Listen
    listen(server_fd, 5);
    printf("Server dang lang nghe...\n");

    while (1) {
        // 5. Accept client
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);

        // 6. fork tạo tiến trình con
        if (fork() == 0) {
            // ---- CHILD PROCESS ----
            close(server_fd); // con không cần server socket

            read(client_fd, buffer, sizeof(buffer));
            printf("Nhan tu client: %s\n", buffer);

            char *msg = "Hello from server\n";
            write(client_fd, msg, strlen(msg));

            close(client_fd);
            exit(0);
        } else {
            // ---- PARENT PROCESS ----
            close(client_fd); // cha không xử lý client
        }
    }

    return 0;
}