#ifndef EVALUATION_H
#define EVALUATION_H
#include "gameState.h"

// Constants for magic numbers
#define MAX_BOARD_SIZE 8
#define MATE_SCORE 100000

int evaluatePosition(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE], GameState* state);

#endif