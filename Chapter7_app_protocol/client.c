#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "protocol.h"
#include "game.h"

static int send_all(int sock, const void *buf, size_t len) {
    const char *p = (const char *)buf;
    size_t total = 0;

    while (total < len) {
        ssize_t n = send(sock, p + total, len - total, 0);
        if (n <= 0) return -1;
        total += (size_t)n;
    }
    return 0;
}

static int recv_all(int sock, void *buf, size_t len) {
    char *p = (char *)buf;
    size_t total = 0;

    while (total < len) {
        ssize_t n = recv(sock, p + total, len - total, 0);
        if (n <= 0) return -1;
        total += (size_t)n;
    }
    return 0;
}

static void handle_state_update(int sock) {
    uint8_t payload[BOARD_SIZE * BOARD_SIZE];
    int board[BOARD_SIZE][BOARD_SIZE];

    if (recv_all(sock, payload, sizeof(payload)) < 0) {
        printf("Mat ket noi khi nhan trang thai ban co.\n");
        exit(1);
    }

    deserialize_board(payload, board);
    printf("Trang thai ban co moi nhat:\n");
    print_board(board);
}

static void handle_result(int sock) {
    uint8_t result_code;

    if (recv_all(sock, &result_code, sizeof(result_code)) < 0) {
        printf("Mat ket noi khi nhan ket qua.\n");
        exit(1);
    }

    if (result_code == RESULT_WIN) {
        printf("Ban THANG!\n");
    } else if (result_code == RESULT_LOSE) {
        printf("Ban THUA!\n");
    } else if (result_code == RESULT_DRAW) {
        printf("Tro choi HOA!\n");
    } else {
        printf("Ket qua khong xac dinh.\n");
    }
}

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in server_addr;

    const char *server_ip = "127.0.0.1";
    if (argc >= 2) {
        server_ip = argv[1];
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock);
        return 1;
    }

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }

    printf("Da ket noi toi server %s:%d\n", server_ip, SERVER_PORT);

    while (1) {
        uint8_t msg_type;

        if (recv_all(sock, &msg_type, sizeof(msg_type)) < 0) {
            printf("Server da dong ket noi.\n");
            break;
        }

        if (msg_type == MSG_TURN_NOTIFICATION) {
            int row, col;
            uint8_t move_msg[3];

            printf("Den luot ban. Nhap hang va cot (0-2 0-2): ");
            fflush(stdout);

            while (scanf("%d %d", &row, &col) != 2) {
                int ch;
                printf("Nhap sai dinh dang. Vui long nhap lai: ");
                while ((ch = getchar()) != '\n' && ch != EOF) {}
            }

            move_msg[0] = MSG_MOVE;
            move_msg[1] = (uint8_t)row;
            move_msg[2] = (uint8_t)col;

            if (send_all(sock, move_msg, sizeof(move_msg)) < 0) {
                printf("Khong gui duoc nuoc di.\n");
                break;
            }
        } else if (msg_type == MSG_STATE_UPDATE) {
            handle_state_update(sock);
        } else if (msg_type == MSG_RESULT) {
            handle_result(sock);
            break;
        } else {
            printf("Nhan duoc thong diep khong hop le: 0x%02X\n", msg_type);
            break;
        }
    }

    close(sock);
    return 0;
}