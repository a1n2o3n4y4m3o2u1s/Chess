#ifndef BOT_H
#define BOT_H

#include "gameState.h"

typedef struct {
    int autoPlay;
    double defaultThinkTime;  // Default thinking time when no time control
} BotSettings;

// Function declarations
void selectBotMove(char board[8][8], int whiteToMove, int* startRow, int* startCol, 
                   int* endRow, int* endCol, GameState* state, double thinkTime, int currentEval);
void getBotMove(char board[8][8], int whiteToMove, int* startRow, int* startCol, 
                int* endRow, int* endCol, GameState* state);
void setBotDepth(int depth);

#endif