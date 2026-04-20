// utils.h
#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>

// Gửi tin nhắn tới tất cả client ngoại trừ sender
void broadcast_message(int sender_fd, int client_sockets[], char usernames[][32], int num_clients, char *message) {
    for (int i = 0; i < num_clients; i++) {
        int fd = client_sockets[i];
        if (fd > 0 && fd != sender_fd) {
            send(fd, message, strlen(message), 0);
        }
    }
}

// Thêm client mới vào mảng
int add_client(int client_sockets[], char usernames[][32], int new_fd, const char *username, int max_clients) {
    for (int i = 0; i < max_clients; i++) {
        if (client_sockets[i] == 0) {
            client_sockets[i] = new_fd;
            strncpy(usernames[i], username, 31);
            usernames[i][31] = '\0';
            return i;
        }
    }
    return -1; // Full
}

// Xóa client khỏi mảng
void remove_client(int client_sockets[], char usernames[][32], int index) {
    client_sockets[index] = 0;
    usernames[index][0] = '\0';
}

#endif