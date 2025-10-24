#ifndef BOARD_H
#define BOARD_H

// Board is 8x8, where [0][0] is a8 (top-left) and [7][7] is h1 (bottom-right)
// Uppercase letters = white pieces, lowercase = black pieces, '.' = empty

void initializeBoard(char board[8][8]);
void printBoard(char board[8][8], int lastStartRow, int lastStartCol, int lastEndRow, int lastEndCol);
int isWhitePiece(char piece);
int isBlackPiece(char piece);
int isEmpty(char piece);

#endif