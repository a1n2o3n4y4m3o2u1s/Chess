#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <gameState.h>
#include <board.h>
#include <moves.h>

void initializeGameState(GameState* state) {
    state->whiteKingsideCastle = 1;
    state->whiteQueensideCastle = 1;
    state->blackKingsideCastle = 1;
    state->blackQueensideCastle = 1;
    state->enPassantCol = -1;
    state->enPassantRow = -1;
}

int isSquareAttacked(char board[8][8], int row, int col, int byWhite, GameState* state) {
    // Check if any piece of the attacking color can move to this square
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            char piece = board[r][c];
            if (isEmpty(piece)) continue;
            
            // Check if piece belongs to attacking color
            int isPieceWhite = isWhitePiece(piece);
            if (isPieceWhite != byWhite) continue;
            
            // For kings, only check adjacent squares (no castling in attack detection)
            if (toupper(piece) == 'K') {
                int rowDiff = abs(row - r);
                int colDiff = abs(col - c);
                if (rowDiff <= 1 && colDiff <= 1) return 1;
                continue;
            }
            
            // Check if this piece can legally move to the target square
            if (canPieceMoveTo(board, r, c, row, col, state)) {
                return 1;
            }
        }
    }
    return 0;
}

int isKingInCheck(char board[8][8], int whiteKing, GameState* state) {
    // Find the king's position
    char kingChar = whiteKing ? 'K' : 'k';
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (board[row][col] == kingChar) {
                // Check if opponent can attack this square
                return isSquareAttacked(board, row, col, !whiteKing, state);
            }
        }
    }
    return 0;  // King not found (shouldn't happen)
}

int doesMovePutKingInCheck(char board[8][8], int startRow, int startCol, 
                           int endRow, int endCol, int whiteToMove, GameState* state) {
    // Save original positions
    char originalStart = board[startRow][startCol];
    char originalEnd = board[endRow][endCol];
    
    // Make the move temporarily
    board[endRow][endCol] = originalStart;
    board[startRow][startCol] = '.';
    
    // Check if king is now in check
    int inCheck = isKingInCheck(board, whiteToMove, state);
    
    // Undo the move
    board[startRow][startCol] = originalStart;
    board[endRow][endCol] = originalEnd;
    
    return inCheck;
}

int hasAnyLegalMoves(char board[8][8], int whiteToMove, GameState* state) {
    // Try every possible move
    for (int startRow = 0; startRow < 8; startRow++) {
        for (int startCol = 0; startCol < 8; startCol++) {
            for (int endRow = 0; endRow < 8; endRow++) {
                for (int endCol = 0; endCol < 8; endCol++) {
                    if (isLegalMove(board, startRow, startCol, endRow, endCol, whiteToMove, state)) {
                        return 1;  // Found at least one legal move
                    }
                }
            }
        }
    }
    return 0;  // No legal moves
}

int checkGameStatus(char board[8][8], int whiteToMove, GameState* state) {
    int inCheck = isKingInCheck(board, whiteToMove, state);
    int hasLegalMoves = hasAnyLegalMoves(board, whiteToMove, state);
    
    if (!hasLegalMoves) {
        if (inCheck) {
            printf("\n*** CHECKMATE! %s wins! ***\n\n", whiteToMove ? "Black" : "White");
        } else {
            printf("\n*** STALEMATE! Draw. ***\n\n");
        }
        return 1;  // Game over
    }
    
    if (inCheck) {
        printf(">>> Check! <<<\n");
    }
    
    return 0;  // Game continues
}