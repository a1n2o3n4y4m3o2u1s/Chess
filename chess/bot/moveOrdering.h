#ifndef MOVE_ORDERING_H
#define MOVE_ORDERING_H

#include <gameState.h>
#include <moves.h>

// Constants for magic numbers
#define MAX_BOARD_SIZE 8
#define MAX_MOVES 256
#define MAX_DEPTH 64
#define KILLERS_PER_DEPTH 2

void clearKillerMoves(void);

void storeKillerMove(Move* move, int depth);

int isKillerMove(Move* move, int depth);

int getCaptureValue(char capturedPiece);

int scoreMoveForOrdering(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE], Move* move, Move* hashMove, int depth);

void sortMoves(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE], Move* moves, int numMoves, Move* hashMove, int depth);

#endif