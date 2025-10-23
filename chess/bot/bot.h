#ifndef BOT_H
#define BOT_H

#include <gameState.h>

extern double BOT_TIME_LIMIT_SECONDS;

// Bot selects best move using minimax with alpha-beta pruning
// Now accepts allocated think time and position evaluation
void selectBotMove(char board[8][8], int whiteToMove, int* startRow, int* startCol, 
                   int* endRow, int* endCol, GameState* state, double thinkTime, int currentEval);

// Configure bot search depth (kept for backward compatibility)
void setBotDepth(int depth);

#endif