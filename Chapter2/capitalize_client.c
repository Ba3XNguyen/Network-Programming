#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char message[256];
    char buffer[BUFFER_SIZE] = {0};

    // Creating socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IP addresses from text to binary
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("Invalid address or address not supported\n");
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection Failed\n");
        return -1;
    }
    // Đọc thông điệp từ user và gửi cho server
    printf("Enter message: ");
    int idx = 0;
    char c;
    while ((c = getchar()) != '\n') {
        message[idx++] = c;
    }
    message[idx] = '\0';

    printf("Send (%s) to server!\n", message);

    // Send a message to the server
    send(sock, message, strlen(message), 0);
    printf("Hello message sent to server\n");

    // Read server's response
    read(sock, buffer, BUFFER_SIZE);
    printf("Message from server: %s\n", buffer);

    // Close the socket
    close(sock);

    return 0;
}