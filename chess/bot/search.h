#ifndef SEARCH_H
#define SEARCH_H

#include <gameState.h>
#include <time.h>
#include <moves.h>

// Constants for magic numbers
#define MAX_BOARD_SIZE 8
#define MAX_MOVES 256
#define INITIAL_ALPHA -999999
#define INITIAL_BETA 999999
#define NODES_BETWEEN_TIME_CHECKS 1000
#define MATE_SCORE 100000
#define MATE_SCORE_THRESHOLD 90000

// UPDATED: Added GameState* parameter to makeMove and unmakeMove
void makeMove(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE], Move* move, char* savedStart, char* savedEnd, 
              char* savedCaptured, int* wasEnPassant, GameState* state);

void unmakeMove(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE], Move* move, char savedStart, char savedEnd, 
                char savedCaptured, int wasEnPassant, GameState* state);

void updateEnPassant(GameState* state, Move* move, char piece);

int quiescenceSearch(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE], GameState* state, int alpha, int beta, 
                     int maximizing, int* nodesEvaluated, clock_t startTime, int ply);

int minimax(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE], GameState* state, int depth, int alpha, int beta, 
            int maximizing, int* nodesEvaluated, unsigned long long hash,
            clock_t startTime, int ply);

#endif