#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define SERVER_PORT 8080
#define BOARD_SIZE 3
#define BUFFER_SIZE 64

// Message types
#define MSG_MOVE              0x02
#define MSG_STATE_UPDATE      0x03
#define MSG_RESULT            0x04
#define MSG_TURN_NOTIFICATION 0x05

// Result payload values
#define RESULT_WIN  0x01
#define RESULT_LOSE 0x02
#define RESULT_DRAW 0x03

// Player IDs
#define PLAYER1 1
#define PLAYER2 2

#endif