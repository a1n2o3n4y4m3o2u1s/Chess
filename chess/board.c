#include <stdio.h>
#include <ctype.h>
#include <board.h>

void initializeBoard(char board[8][8]) {
    // Black pieces on ranks 7-8
    char* blackPieces = "rnbqkbnr";
    for (int col = 0; col < 8; col++) {
        board[0][col] = blackPieces[col];
        board[1][col] = 'p';
    }
    
    // Empty squares in the middle
    for (int row = 2; row < 6; row++) {
        for (int col = 0; col < 8; col++) {
            board[row][col] = '.';
        }
    }
    
    // White pieces on ranks 1-2
    char* whitePieces = "RNBQKBNR";
    for (int col = 0; col < 8; col++) {
        board[6][col] = 'P';
        board[7][col] = whitePieces[col];
    }
}

void printBoard(char board[8][8]) {
    printf("\n");
    for (int row = 0; row < 8; row++) {
        printf("%d     ", 8 - row);  // Rank numbers (8 to 1)
        
        for (int col = 0; col < 8; col++) {
            printf("%c   ", board[row][col]);
        }
        printf("\n\n");
    }
    printf("\n      a   b   c   d   e   f   g   h\n\n");  // File letters
}

int isWhitePiece(char piece) {
    return isupper(piece) && piece != '.';
}

int isBlackPiece(char piece) {
    return islower(piece) && piece != '.';
}

int isEmpty(char piece) {
    return piece == '.';
}