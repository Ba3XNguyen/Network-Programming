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

static int send_turn_notification(int sock) {
    uint8_t msg[1];
    msg[0] = MSG_TURN_NOTIFICATION;
    return send_all(sock, msg, sizeof(msg));
}

static int send_state_update(int sock, const int board[BOARD_SIZE][BOARD_SIZE]) {
    uint8_t msg[1 + BOARD_SIZE * BOARD_SIZE];
    msg[0] = MSG_STATE_UPDATE;
    serialize_board(board, msg + 1);
    return send_all(sock, msg, sizeof(msg));
}

static int send_result(int sock, uint8_t result_code) {
    uint8_t msg[2];
    msg[0] = MSG_RESULT;
    msg[1] = result_code;
    return send_all(sock, msg, sizeof(msg));
}

int main(void) {
    int server_sock = -1;
    int client1_sock = -1;
    int client2_sock = -1;

    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int board[BOARD_SIZE][BOARD_SIZE];
    int move_count = 0;
    int current_player = PLAYER1;

    init_board(board);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(server_sock);
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_sock);
        return 1;
    }

    if (listen(server_sock, 2) < 0) {
        perror("listen");
        close(server_sock);
        return 1;
    }

    printf("Server dang lang nghe tai cong %d...\n", SERVER_PORT);
    printf("Cho nguoi choi 1 ket noi...\n");
    client1_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
    if (client1_sock < 0) {
        perror("accept client1");
        close(server_sock);
        return 1;
    }
    printf("Nguoi choi 1 da ket noi.\n");

    printf("Cho nguoi choi 2 ket noi...\n");
    client2_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
    if (client2_sock < 0) {
        perror("accept client2");
        close(client1_sock);
        close(server_sock);
        return 1;
    }
    printf("Nguoi choi 2 da ket noi.\n");
    printf("Bat dau tro choi!\n");

    while (1) {
        int current_sock = (current_player == PLAYER1) ? client1_sock : client2_sock;
        int other_sock   = (current_player == PLAYER1) ? client2_sock : client1_sock;

        if (send_turn_notification(current_sock) < 0) {
            printf("Mat ket noi voi nguoi choi %d.\n", current_player);
            break;
        }

        while (1) {
            uint8_t move_msg[3];
            int row, col;
            int winner;

            if (recv_all(current_sock, move_msg, sizeof(move_msg)) < 0) {
                printf("Khong nhan duoc nuoc di tu nguoi choi %d.\n", current_player);
                goto cleanup;
            }

            if (move_msg[0] != MSG_MOVE) {
                printf("Thong diep khong hop le tu nguoi choi %d.\n", current_player);
                goto cleanup;
            }

            row = (int)move_msg[1];
            col = (int)move_msg[2];

            if (!is_valid_move(board, row, col)) {
                printf("Nguoi choi %d gui nuoc di khong hop le: (%d, %d)\n",
                       current_player, row, col);

                // Theo de: bo qua nuoc di khong hop le va yeu cau thu lai
                if (send_turn_notification(current_sock) < 0) {
                    goto cleanup;
                }
                continue;
            }

            apply_move(board, row, col, current_player);
            move_count++;

            printf("Nguoi choi %d danh vao (%d, %d)\n", current_player, row, col);
            print_board(board);

            if (send_state_update(client1_sock, board) < 0 ||
                send_state_update(client2_sock, board) < 0) {
                printf("Loi gui trang thai ban co.\n");
                goto cleanup;
            }

            winner = check_winner(board);
            if (winner == PLAYER1) {
                send_result(client1_sock, RESULT_WIN);
                send_result(client2_sock, RESULT_LOSE);
                printf("Nguoi choi 1 thang!\n");
                goto cleanup;
            } else if (winner == PLAYER2) {
                send_result(client1_sock, RESULT_LOSE);
                send_result(client2_sock, RESULT_WIN);
                printf("Nguoi choi 2 thang!\n");
                goto cleanup;
            } else if (is_draw(board, move_count)) {
                send_result(client1_sock, RESULT_DRAW);
                send_result(client2_sock, RESULT_DRAW);
                printf("Tro choi hoa!\n");
                goto cleanup;
            }

            current_player = (current_player == PLAYER1) ? PLAYER2 : PLAYER1;
            break;
        }

        (void)other_sock;
    }

cleanup:
    if (client1_sock >= 0) close(client1_sock);
    if (client2_sock >= 0) close(client2_sock);
    if (server_sock >= 0) close(server_sock);

    return 0;
}