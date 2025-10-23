#ifndef MOVES_H
#define MOVES_H

#include <gameState.h>

// Move structure for bot move generation
typedef struct {
    int startRow;
    int startCol;
    int endRow;
    int endCol;
    char promotionPiece;  // ADD THIS: 'Q', 'R', 'B', 'N', or 0 for none
} Move;

// Core move validation
int isLegalMove(char board[8][8], int startRow, int startCol, int endRow, int endCol, 
                int whiteToMove, GameState* state);

// Helper function for attack detection
int canPieceMoveTo(char board[8][8], int startRow, int startCol, int endRow, int endCol, 
                   GameState* state);

// Piece-specific movement rules
int isValidPawnMove(char board[8][8], int startRow, int startCol, int endRow, int endCol, 
                    GameState* state);
int isValidKnightMove(int startRow, int startCol, int endRow, int endCol);
int isValidBishopMove(char board[8][8], int startRow, int startCol, int endRow, int endCol);
int isValidRookMove(char board[8][8], int startRow, int startCol, int endRow, int endCol);
int isValidQueenMove(char board[8][8], int startRow, int startCol, int endRow, int endCol);
int isValidKingMove(char board[8][8], int startRow, int startCol, int endRow, int endCol, 
                    GameState* state);

// Move generation for bot
int generateAllLegalMoves(char board[8][8], int whiteToMove, Move moves[], GameState* state);

// ADD THIS: Promotion validation helper
int isLegalMoveWithPromotion(char board[8][8], int startRow, int startCol, int endRow, int endCol, 
                            int whiteToMove, GameState* state, char promotionPiece);

#endif