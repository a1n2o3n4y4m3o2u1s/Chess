#ifndef GAME_STATE_H
#define GAME_STATE_H

// Tracks castling rights and en passant opportunities
typedef struct {
    int whiteKingsideCastle;
    int whiteQueensideCastle;
    int blackKingsideCastle;
    int blackQueensideCastle;
    int enPassantCol;  // -1 if no en passant available
    int enPassantRow;
} GameState;

// Initialize game state with default values
void initializeGameState(GameState* state);

// Check detection
int isSquareAttacked(char board[8][8], int row, int col, int byWhite, GameState* state);
int isKingInCheck(char board[8][8], int whiteKing, GameState* state);
int doesMovePutKingInCheck(char board[8][8], int startRow, int startCol, 
                           int endRow, int endCol, int whiteToMove, GameState* state);

// Game status
int hasAnyLegalMoves(char board[8][8], int whiteToMove, GameState* state);
int checkGameStatus(char board[8][8], int whiteToMove, GameState* state);

#endif