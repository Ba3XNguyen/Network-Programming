#include <stdio.h>
#include "game.h"

void init_board(int board[BOARD_SIZE][BOARD_SIZE]) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            board[i][j] = 0;
        }
    }
}

void print_board(const int board[BOARD_SIZE][BOARD_SIZE]) {
    printf("\n");
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            char c = '.';
            if (board[i][j] == PLAYER1) c = 'X';
            else if (board[i][j] == PLAYER2) c = 'O';

            printf(" %c ", c);
            if (j < BOARD_SIZE - 1) printf("|");
        }
        printf("\n");
        if (i < BOARD_SIZE - 1) {
            printf("---+---+---\n");
        }
    }
    printf("\n");
}

int is_valid_move(const int board[BOARD_SIZE][BOARD_SIZE], int row, int col) {
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
        return 0;
    }
    return board[row][col] == 0;
}

int apply_move(int board[BOARD_SIZE][BOARD_SIZE], int row, int col, int player) {
    if (!is_valid_move(board, row, col)) {
        return 0;
    }
    board[row][col] = player;
    return 1;
}

int check_winner(const int board[BOARD_SIZE][BOARD_SIZE]) {
    // Rows
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (board[i][0] != 0 &&
            board[i][0] == board[i][1] &&
            board[i][1] == board[i][2]) {
            return board[i][0];
        }
    }

    // Columns
    for (int j = 0; j < BOARD_SIZE; j++) {
        if (board[0][j] != 0 &&
            board[0][j] == board[1][j] &&
            board[1][j] == board[2][j]) {
            return board[0][j];
        }
    }

    // Diagonal 1
    if (board[0][0] != 0 &&
        board[0][0] == board[1][1] &&
        board[1][1] == board[2][2]) {
        return board[0][0];
    }

    // Diagonal 2
    if (board[0][2] != 0 &&
        board[0][2] == board[1][1] &&
        board[1][1] == board[2][0]) {
        return board[0][2];
    }

    return 0;
}

int is_draw(const int board[BOARD_SIZE][BOARD_SIZE], int move_count) {
    (void)board;
    return move_count >= BOARD_SIZE * BOARD_SIZE && check_winner(board) == 0;
}

void serialize_board(const int board[BOARD_SIZE][BOARD_SIZE], uint8_t *out) {
    int idx = 0;
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            out[idx++] = (uint8_t)board[i][j];
        }
    }
}

void deserialize_board(const uint8_t *in, int board[BOARD_SIZE][BOARD_SIZE]) {
    int idx = 0;
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            board[i][j] = (int)in[idx++];
        }
    }
}