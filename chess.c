#include <stdio.h>
#include <string.h>

void printBoard(char board[8][8]) {
    printf("\n");
    for (int i = 0; i < 8; i++) {
        printf("%d     ", 8 - i);  // Print ranks on the left
        for (int k = 0; k < 8; k++) {
            printf("%c   ", board[i][k]);
        }
        printf("\n\n");
    }
    printf("\n      a   b   c   d   e   f   g   h\n\n");  // Print files at the bottom
}

int rightColorMoving (char board[8][8], int startRow, int startCol, int whiteToMove) {
    char piece = board[startRow][startCol];
    if (whiteToMove == 1) {
        if (strchr("PRNBQK", piece)) {
            return 1;
        } else {
            printf("A white piece is supposed to move!\n");
            return 0;
        }
    } else {
        if (strchr("prnbqk", piece)) {
            return 1;
        } else {
            printf("A black piece is supposed to move!\n");
            return 0;
        }
    }
}

int noSelfTake (char board[8][8], int endRow, int endCol, int whiteToMove) {
    char targetPiece = board[endRow][endCol];
    if (whiteToMove == 1) {
        if (strchr("PRNBQK", targetPiece)) {
            printf("You can't take your own piece\n");
            return 0;
        } else {
            return 1;
        }
    } else {
        if (strchr("prnbqk", targetPiece)) {
            printf("You can't take your own piece\n");
            return 0;
        } else {
            return 1;
        }
    }
}

int isLegalMove(char board[8][8], int startRow, int startCol, int endRow, int endCol, int whiteToMove) {
    if (rightColorMoving(board, startRow, startCol, whiteToMove) && noSelfTake(board, endRow, endCol, whiteToMove)) {
        return 1;
    } else {
        return 0;
    }
}

int main() {

    setvbuf(stdout, NULL, _IONBF, 0);

    char board[8][8] = {
    {'r','n','b','q','k','b','n','r'},
    {'p','p','p','p','p','p','p','p'},
    {'.','.','.','.','.','.','.','.'},
    {'.','.','.','.','.','.','.','.'},
    {'.','.','.','.','.','.','.','.'},
    {'.','.','.','.','.','.','.','.'},
    {'P','P','P','P','P','P','P','P'},
    {'R','N','B','Q','K','B','N','R'}
    };


    char move[5];

    int whiteToMove = 1;

    while (move[0] != 's') {

        printBoard(board);

        scanf("%4s", move);
        printf("You entered: %s\n", move);

        int startCol = move[0] - 'a';        // 'a'→0, 'h'→7
        int startRow = 8 - (move[1] - '0');  // '1'→7, '8'→0
        int endCol   = move[2] - 'a';
        int endRow   = 8 - (move[3] - '0');

        // Call your legality checker
        if (isLegalMove(board, startRow, startCol, endRow, endCol, whiteToMove) == 1) {
            // Legal move → execute it
            printf("Move executed: %c from %c%d to %c%d\n", board[startRow][startCol], 'a' + startCol, 8 - startRow, 'a' + endCol, 8 - endRow);
            board[endRow][endCol] = board[startRow][startCol];
            board[startRow][startCol] = '.';
            if (whiteToMove == 1) {
                printf("Enter next move (Black to move):\n");
                whiteToMove = 0;
            } else {
                printf("Enter next move (White to move):\n");
                whiteToMove = 1;
            }
        } else {
            // Illegal move → reject it
            printf("Illegal move! \nTry again, ");
            if (whiteToMove == 1) {
                printf("Enter next move (White to move):\n");
            } else {
                printf("Enter next move (Black to move):\n");
            }
        }
    }
};