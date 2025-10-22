#ifndef BOT_H
#define BOT_H

#include <gameState.h>

extern double BOT_TIME_LIMIT_SECONDS;

// Bot selects best move using minimax with alpha-beta pruning
void selectBotMove(char board[8][8], int whiteToMove, int* startRow, int* startCol, 
                   int* endRow, int* endCol, GameState* state);

// Configure bot search depth (higher = stronger but slower)
void setBotDepth(int depth);

#endif