#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080

void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    free(arg); // giải phóng bộ nhớ

    char buffer[1024] = {0};
    read(client_fd, buffer, sizeof(buffer));
    printf("Nhan tu client: %s\n", buffer);

    char *msg = "Hello from server\n";
    write(client_fd, msg, strlen(msg));

    close(client_fd);
    return NULL;
}

int main() {
    int server_fd;
    struct sockaddr_in server_addr;

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
        int *client_fd = malloc(sizeof(int));

        // 5. Accept client
        *client_fd = accept(server_fd, NULL, NULL);

        pthread_t tid;

        // 6. Tạo thread xử lý client
        pthread_create(&tid, NULL, handle_client, client_fd);

        pthread_detach(tid); // không cần join
    }

    return 0;
}