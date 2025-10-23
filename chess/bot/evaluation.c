#include <evaluation.h>
#include <ctype.h>
#include <stdlib.h>  // For abs
#include <board.h> 

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
        {-10,  0, 10, 10, 10,  10, 0,-10},
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
    
    // NEW: Rook position bonuses
    static const int rookTable[MAX_BOARD_SIZE][MAX_BOARD_SIZE] = {
        { 0,  0,  0,  5,  5,  0,  0,  0},
        {-5,  0,  0,  0,  0,  0,  0, -5},
        {-5,  0,  0,  0,  0,  0,  0, -5},
        {-5,  0,  0,  0,  0,  0,  0, -5},
        {-5,  0,  0,  0,  0,  0,  0, -5},
        {-5,  0,  0,  0,  0,  0,  0, -5},
        { 5, 10, 10, 10, 10, 10, 10,  5},
        { 0,  0,  0,  0,  0,  0,  0,  0}
    };
    
    // CHANGED: Single pass instead of multiple passes
    int whiteMinorCount = 0, blackMinorCount = 0;
    int whiteMajorCount = 0, blackMajorCount = 0;
    int whiteBishopCount = 0, blackBishopCount = 0;
    int whitePawnCount = 0, blackPawnCount = 0;
    int whitePawnsOnFile[8] = {0};  // NEW: For doubled pawn detection
    int blackPawnsOnFile[8] = {0};
    
    // CHANGED: Combined counting and evaluation into single pass
    for (int row = 0; row < MAX_BOARD_SIZE; row++) {
        for (int col = 0; col < MAX_BOARD_SIZE; col++) {
            char piece = board[row][col];
            if (isEmpty(piece)) continue;
            
            char pieceType = toupper(piece);
            int value = pieceValues[(int)piece];
            int positionalBonus = 0;
            int isWhite = isWhitePiece(piece);
            
            // Count pieces for various evaluations
            if (isWhite) {
                switch (pieceType) {
                    case 'N': case 'B': 
                        whiteMinorCount++;
                        if (pieceType == 'B') whiteBishopCount++;
                        break;
                    case 'R': case 'Q': 
                        whiteMajorCount++; 
                        break;
                    case 'P':
                        whitePawnCount++;
                        whitePawnsOnFile[col]++;  // Track pawn files
                        break;
                }
            } else {
                switch (pieceType) {
                    case 'N': case 'B': 
                        blackMinorCount++;
                        if (pieceType == 'B') blackBishopCount++;
                        break;
                    case 'R': case 'Q': 
                        blackMajorCount++; 
                        break;
                    case 'P':
                        blackPawnCount++;
                        blackPawnsOnFile[col]++;  // Track pawn files
                        break;
                }
            }
            
            // Positional evaluation
            if (isWhite) {
                score += value;
                int flippedRow = MAX_BOARD_SIZE - 1 - row;
                
                switch (pieceType) {
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
                        positionalBonus = rookTable[flippedRow][col];
                        // NEW: Rook on open file bonus
                        if (whitePawnsOnFile[col] == 0) {
                            positionalBonus += 15;
                        }
                        // NEW: Rook on 7th rank bonus
                        if (flippedRow == 1) {  // 7th rank for white
                            positionalBonus += 20;
                        }
                        break;
                    case 'Q':
                        // NEW: Keep queen safe in early game
                        if (whiteMinorCount + whiteMajorCount > 8) {
                            positionalBonus = -abs(col - 3) - abs(flippedRow - 3);
                        }
                        break;
                    case 'K':
                        // FIXED: Move variable declaration outside switch
                        if (whiteMinorCount + whiteMajorCount + whitePawnCount > 10) {  // Middlegame
                            positionalBonus = kingTable[flippedRow][col];
                        } else {  // Endgame
                            positionalBonus = -abs(col - 3) - abs(flippedRow - 3);
                        }
                        break;
                }
                score += positionalBonus;
                
            } else {  // Black pieces
                score -= value;
                
                switch (pieceType) {
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
                        positionalBonus = rookTable[row][col];
                        // NEW: Rook on open file bonus
                        if (blackPawnsOnFile[col] == 0) {
                            positionalBonus += 15;
                        }
                        // NEW: Rook on 7th rank bonus (rank 1 for black)
                        if (row == 6) {  // 7th rank for black
                            positionalBonus += 20;
                        }
                        break;
                    case 'Q':
                        // NEW: Keep queen safe in early game
                        if (blackMinorCount + blackMajorCount > 8) {
                            positionalBonus = -abs(col - 3) - abs(row - 3);
                        }
                        break;
                    case 'K':
                        // FIXED: Move variable declaration outside switch
                        if (blackMinorCount + blackMajorCount + blackPawnCount > 10) {  // Middlegame
                            positionalBonus = kingTable[row][col];
                        } else {  // Endgame
                            positionalBonus = -abs(col - 3) - abs(row - 3);
                        }
                        break;
                }
                score -= positionalBonus;
            }
        }
    }
    
    // NEW: Doubled pawn penalty (simple but effective)
    for (int col = 0; col < MAX_BOARD_SIZE; col++) {
        if (whitePawnsOnFile[col] > 1) score -= 10 * (whitePawnsOnFile[col] - 1);
        if (blackPawnsOnFile[col] > 1) score += 10 * (blackPawnsOnFile[col] - 1);
    }
    
    // NEW: Isolated pawn penalty (simple version)
    for (int col = 0; col < MAX_BOARD_SIZE; col++) {
        int whiteIsolated = (whitePawnsOnFile[col] > 0) &&
                           (col == 0 || whitePawnsOnFile[col-1] == 0) &&
                           (col == MAX_BOARD_SIZE-1 || whitePawnsOnFile[col+1] == 0);
        int blackIsolated = (blackPawnsOnFile[col] > 0) &&
                           (col == 0 || blackPawnsOnFile[col-1] == 0) &&
                           (col == MAX_BOARD_SIZE-1 || blackPawnsOnFile[col+1] == 0);
        
        if (whiteIsolated) score -= 15;
        if (blackIsolated) score += 15;
    }
    
    // Bishop pair bonuses
    if (whiteBishopCount >= 2) score += 50;
    if (blackBishopCount >= 2) score -= 50;
    
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