#ifndef TRANSPOSITION_H
#define TRANSPOSITION_H

#include <gameState.h>
#include <moves.h>

// Constants for magic numbers
#define MAX_BOARD_SIZE 8
#define MAX_PIECE_TYPES 12
#define INITIAL_SEED 12345

// Transposition table entry
typedef struct {
    unsigned long long hash;
    int depth;
    int score;
    int flag;  // 0 = exact, 1 = lower bound (alpha), 2 = upper bound (beta)
    Move bestMove;
} TTEntry;

#define TT_SIZE 1048576  // 1M entries (about 32MB)
#define TT_EXACT 0
#define TT_ALPHA 1
#define TT_BETA 2

// Initialize Zobrist random numbers
void initZobrist(void);

// Map piece character to index (0-11)
int pieceToIndex(char piece);

// Compute hash for current board position
unsigned long long computeHash(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE]);

// Initialize transposition table
int initTranspositionTable(void);

// Free transposition table
void freeTranspositionTable(void);

// Probe transposition table
TTEntry* probeTranspositionTable(unsigned long long hash);

// Store in transposition table
void storeTranspositionTable(unsigned long long hash, int depth, int score, 
                             int flag, Move* bestMove);

#endif