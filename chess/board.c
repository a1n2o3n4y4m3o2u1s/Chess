#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define COLOR_WHITE_SQ "\x1b[48;5;255m"  // White squares
#define COLOR_BLACK_SQ  "\x1b[48;5;0m"   // Black squares
#define COLOR_HIGHLIGHT_START "\x1b[48;5;230m"  // Slightly yellower white for starting square
#define COLOR_HIGHLIGHT_END "\x1b[48;5;58m"     // Slightly yellower black for ending square
#define COLOR_RESET "\x1b[0m"

// Function declarations
int isEmpty(char piece);
int isWhitePiece(char piece);
int isBlackPiece(char piece);

const char* getPieceSymbol(char piece, int is_white_square) {
    // For white pieces (uppercase)
    switch(piece) {
        case 'P': return is_white_square ? "♙" : "♟";  // White pawn on white vs black
        case 'N': return is_white_square ? "♘" : "♞";  // White knight
        case 'B': return is_white_square ? "♗" : "♝";  // White bishop
        case 'R': return is_white_square ? "♖" : "♜";  // White rook
        case 'Q': return is_white_square ? "♕" : "♛";  // White queen
        case 'K': return is_white_square ? "♔" : "♚";  // White king
        
        // For black pieces (lowercase) - inverse logic
        case 'p': return is_white_square ? "♟" : "♙";  // Black pawn on white vs black
        case 'n': return is_white_square ? "♞" : "♘";  // Black knight
        case 'b': return is_white_square ? "♝" : "♗";  // Black bishop
        case 'r': return is_white_square ? "♜" : "♖";  // Black rook
        case 'q': return is_white_square ? "♛" : "♕";  // Black queen
        case 'k': return is_white_square ? "♚" : "♔";  // Black king
        
        default: return " ";
    }
}

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

void printBoard(char board[8][8], int lastStartRow, int lastStartCol, int lastEndRow, int lastEndCol) {
    const int cell_width = 5;
    const int cell_height = 3;
    const int center_offset = (cell_width + 1) / 2;
    const int rank_width = center_offset * 2;
    const int leading_spaces = rank_width + center_offset - 1;
    const int inter_spaces = cell_width - 1;
    const int trailing_spaces = cell_width - center_offset;

    printf("\n");
    for (int row = 0; row < 8; row++) {
        for (int sub = 0; sub < cell_height; sub++) {
            // Print rank number only on the middle sub-row
            int is_middle_sub = (sub == cell_height / 2);
            if (is_middle_sub) {
                printf("%d%*s", 8 - row, rank_width - 1, "");
            } else {
                printf("%*s", rank_width, "");
            }

            for (int col = 0; col < 8; col++) {
                char piece = board[row][col];
                int is_white_square = ((row + col) % 2 == 0);
                const char* sym = getPieceSymbol(piece, is_white_square);
                const char* bg_color = is_white_square ? COLOR_WHITE_SQ : COLOR_BLACK_SQ;

                // Override background for highlighted squares
                if (row == lastStartRow && col == lastStartCol) {
                    bg_color = COLOR_HIGHLIGHT_START;
                } else if (row == lastEndRow && col == lastEndCol) {
                    bg_color = COLOR_HIGHLIGHT_END;
                }

                printf("%s", bg_color);

                if (!is_middle_sub || isEmpty(piece)) {
                    printf("%*s", cell_width, "");
                } else {
                    int left_pad = (cell_width - 1) / 2;
                    int right_pad = cell_width - 1 - left_pad;
                    printf("%*s%s%*s", left_pad, "", sym, right_pad, "");
                }
            }
            printf("%s\n", COLOR_RESET);
        }
    }
    printf("\n%*s", leading_spaces, "");
    for (int i = 0; i < 8; i++) {
        printf("%c", 'a' + i);
        if (i < 7) {
            printf("%*s", inter_spaces, "");
        }
    }
    printf("%*s\n\n", trailing_spaces, "");
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