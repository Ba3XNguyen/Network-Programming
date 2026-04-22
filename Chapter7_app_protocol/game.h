#ifndef GAME_H
#define GAME_H

#include "protocol.h"

void init_board(int board[BOARD_SIZE][BOARD_SIZE]);
void print_board(const int board[BOARD_SIZE][BOARD_SIZE]);
int is_valid_move(const int board[BOARD_SIZE][BOARD_SIZE], int row, int col);
int apply_move(int board[BOARD_SIZE][BOARD_SIZE], int row, int col, int player);
int check_winner(const int board[BOARD_SIZE][BOARD_SIZE]);
int is_draw(const int board[BOARD_SIZE][BOARD_SIZE], int move_count);
void serialize_board(const int board[BOARD_SIZE][BOARD_SIZE], uint8_t *out);
void deserialize_board(const uint8_t *in, int board[BOARD_SIZE][BOARD_SIZE]);

#endif