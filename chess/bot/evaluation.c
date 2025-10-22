#include <evaluation.h>
#include <ctype.h>
#include <stdlib.h>  // For abs

int evaluatePosition(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE]) {
    int score = 0;
    
    // Material values (centipawns)
    int pieceValues[256] = {0};
    pieceValues['P'] = 100;   pieceValues['p'] = 100;
    pieceValues['N'] = 320;   pieceValues['n'] = 320;
    pieceValues['B'] = 330;   pieceValues['b'] = 330;
    pieceValues['R'] = 500;   pieceValues['r'] = 500;
    pieceValues['Q'] = 900;   pieceValues['q'] = 900;
    pieceValues['K'] = 20000; pieceValues['k'] = 20000;
    
    // Pawn structure bonuses
    static const int pawnTable[MAX_BOARD_SIZE][MAX_BOARD_SIZE] = {
        { 0,  0,  0,  0,  0,  0,  0,  0},
        {50, 50, 50, 50, 50, 50, 50, 50},
        {10, 10, 20, 30, 30, 20, 10, 10},
        { 5,  5, 10, 25, 25, 10,  5,  5},
        { 0,  0,  0, 20, 20,  0,  0,  0},
        { 5, -5,-10,  0,  0,-10, -5,  5},
        { 5, 10, 10,-20,-20, 10, 10,  5},
        { 0,  0,  0,  0,  0,  0,  0,  0}
    };
    
    // Knight position bonuses
    static const int knightTable[MAX_BOARD_SIZE][MAX_BOARD_SIZE] = {
        {-50,-40,-30,-30,-30,-30,-40,-50},
        {-40,-20,  0,  0,  0,  0,-20,-40},
        {-30,  0, 10, 15, 15, 10,  0,-30},
        {-30,  5, 15, 20, 20, 15,  5,-30},
        {-30,  0, 15, 20, 20, 15,  0,-30},
        {-30,  5, 10, 15, 15, 10,  5,-30},
        {-40,-20,  0,  5,  5,  0,-20,-40},
        {-50,-40,-30,-30,-30,-30,-40,-50}
    };
    
    // Bishop position bonuses
    static const int bishopTable[MAX_BOARD_SIZE][MAX_BOARD_SIZE] = {
        {-20,-10,-10,-10,-10,-10,-10,-20},
        {-10,  0,  0,  0,  0,  0,  0,-10},
        {-10,  0,  5, 10, 10,  5,  0,-10},
        {-10,  5,  5, 10, 10,  5,  5,-10},
        {-10,  0, 10, 10, 10, 10,  0,-10},
        {-10, 10, 10, 10, 10, 10, 10,-10},
        {-10,  5,  0,  0,  0,  0,  5,-10},
        {-20,-10,-10,-10,-10,-10,-10,-20}
    };
    
    // King safety (middle game)
    static const int kingTable[MAX_BOARD_SIZE][MAX_BOARD_SIZE] = {
        {-30,-40,-40,-50,-50,-40,-40,-30},
        {-30,-40,-40,-50,-50,-40,-40,-30},
        {-30,-40,-40,-50,-50,-40,-40,-30},
        {-30,-40,-40,-50,-50,-40,-40,-30},
        {-20,-30,-30,-40,-40,-30,-30,-20},
        {-10,-20,-20,-20,-20,-20,-20,-10},
        { 20, 20,  0,  0,  0,  0, 20, 20},
        { 20, 30, 10,  0,  0, 10, 30, 20}
    };
    
    // First pass: count pieces for bishop pair, minors, majors
    int whiteMinorCount = 0, blackMinorCount = 0;
    int whiteMajorCount = 0, blackMajorCount = 0;
    int whiteBishopCount = 0, blackBishopCount = 0;
    
    for (int row = 0; row < MAX_BOARD_SIZE; row++) {
        for (int col = 0; col < MAX_BOARD_SIZE; col++) {
            char piece = board[row][col];
            if (isEmpty(piece)) continue;
            
            switch (toupper(piece)) {
                case 'N':
                case 'B':
                    if (isWhitePiece(piece)) {
                        whiteMinorCount++;
                        if (toupper(piece) == 'B') whiteBishopCount++;
                    } else {
                        blackMinorCount++;
                        if (toupper(piece) == 'B') blackBishopCount++;
                    }
                    break;
                case 'R':
                case 'Q':
                    if (isWhitePiece(piece)) {
                        whiteMajorCount++;
                    } else {
                        blackMajorCount++;
                    }
                    break;
            }
        }
    }
    
    // Bishop pair bonuses
    if (whiteBishopCount >= 2) score += 50;
    if (blackBishopCount >= 2) score -= 50;
    
    // Second pass: material and positional
    for (int row = 0; row < MAX_BOARD_SIZE; row++) {
        for (int col = 0; col < MAX_BOARD_SIZE; col++) {
            char piece = board[row][col];
            if (isEmpty(piece)) continue;
            
            int value = pieceValues[(int)piece];
            int positionalBonus = 0;
            
            if (isWhitePiece(piece)) {
                score += value;
                int flippedRow = MAX_BOARD_SIZE - 1 - row;
                
                switch (toupper(piece)) {
                    case 'P': 
                        positionalBonus = pawnTable[flippedRow][col];
                        break;
                    case 'N':
                        positionalBonus = knightTable[flippedRow][col];
                        break;
                    case 'B':
                        positionalBonus = bishopTable[flippedRow][col];
                        break;
                    case 'R':
                        positionalBonus = (flippedRow == MAX_BOARD_SIZE - 1) ? 10 : 0;
                        break;
                    case 'Q':
                        break;
                    case 'K':
                        if (whiteMinorCount + whiteMajorCount > 6) {
                            positionalBonus = kingTable[flippedRow][col];
                        } else {
                            positionalBonus = -abs(col - 3) - abs(flippedRow - 3);
                        }
                        break;
                }
                score += positionalBonus;
                
            } else {
                score -= value;
                
                switch (toupper(piece)) {
                    case 'P': 
                        positionalBonus = pawnTable[row][col];
                        break;
                    case 'N':
                        positionalBonus = knightTable[row][col];
                        break;
                    case 'B':
                        positionalBonus = bishopTable[row][col];
                        break;
                    case 'R':
                        positionalBonus = (row == 0) ? 10 : 0;
                        break;
                    case 'Q':
                        break;
                    case 'K':
                        if (blackMinorCount + blackMajorCount > 6) {
                            positionalBonus = kingTable[row][col];
                        } else {
                            positionalBonus = -abs(col - 3) - abs(row - 3);
                        }
                        break;
                }
                score -= positionalBonus;
            }
        }
    }
    
    // Center control bonus
    for (int row = 3; row <= 4; row++) {
        for (int col = 3; col <= 4; col++) {
            char piece = board[row][col];
            if (!isEmpty(piece)) {
                if (isWhitePiece(piece)) score += 5;
                else score -= 5;
            }
        }
    }
    
    return score;
}