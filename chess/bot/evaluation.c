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
    
    // Pawn structure bonuses (symmetric)
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
    
    // Knight position bonuses (symmetric)
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
    
    // Bishop position bonuses (symmetric)
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
    
    // King safety (middle game) - FIXED: Now properly symmetric
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
    
    // King safety (endgame) - NEW: Separate endgame king table
    static const int kingTableEndgame[MAX_BOARD_SIZE][MAX_BOARD_SIZE] = {
        {-50,-40,-30,-20,-20,-30,-40,-50},
        {-30,-20,-10,  0,  0,-10,-20,-30},
        {-30,-10, 20, 30, 30, 20,-10,-30},
        {-30,-10, 30, 40, 40, 30,-10,-30},
        {-30,-10, 30, 40, 40, 30,-10,-30},
        {-30,-10, 20, 30, 30, 20,-10,-30},
        {-30,-30,  0,  0,  0,  0,-30,-30},
        {-50,-30,-30,-30,-30,-30,-30,-50}
    };
    
    // Rook position bonuses (symmetric)
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
    
    // Piece counts
    int whiteMinorCount = 0, blackMinorCount = 0;
    int whiteMajorCount = 0, blackMajorCount = 0;
    int whiteBishopCount = 0, blackBishopCount = 0;
    int whitePawnCount = 0, blackPawnCount = 0;
    int whitePawnsOnFile[8] = {0};
    int blackPawnsOnFile[8] = {0};
    
    // First pass: count pieces for game phase detection
    for (int row = 0; row < MAX_BOARD_SIZE; row++) {
        for (int col = 0; col < MAX_BOARD_SIZE; col++) {
            char piece = board[row][col];
            if (isEmpty(piece)) continue;
            
            char pieceType = toupper(piece);
            if (isWhitePiece(piece)) {
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
                        whitePawnsOnFile[col]++;
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
                        blackPawnsOnFile[col]++;
                        break;
                }
            }
        }
    }
    
    // Calculate game phase for both sides independently
    int whiteGamePhase = (whiteMinorCount + whiteMajorCount + whitePawnCount);
    int blackGamePhase = (blackMinorCount + blackMajorCount + blackPawnCount);
    int whiteEndgame = (whiteGamePhase <= 10);
    int blackEndgame = (blackGamePhase <= 10);
    
    // Second pass: evaluate pieces with proper symmetry
    for (int row = 0; row < MAX_BOARD_SIZE; row++) {
        for (int col = 0; col < MAX_BOARD_SIZE; col++) {
            char piece = board[row][col];
            if (isEmpty(piece)) continue;
            
            char pieceType = toupper(piece);
            int value = pieceValues[(int)piece];
            int positionalBonus = 0;
            int isWhite = isWhitePiece(piece);
            
            if (isWhite) {
                score += value;
                int flippedRow = MAX_BOARD_SIZE - 1 - row;  // Mirror for white
                
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
                        // Rook on open file bonus
                        if (whitePawnsOnFile[col] == 0) {
                            positionalBonus += 15;
                        }
                        // Rook on 7th rank bonus
                        if (flippedRow == 1) {
                            positionalBonus += 20;
                        }
                        break;
                    case 'Q':
                        // Keep queen safe in early game
                        if (!whiteEndgame) {
                            positionalBonus = -abs(col - 3) - abs(flippedRow - 3);
                        }
                        break;
                    case 'K':
                        if (whiteEndgame) {
                            positionalBonus = kingTableEndgame[flippedRow][col];
                        } else {
                            positionalBonus = kingTable[flippedRow][col];
                        }
                        break;
                }
                score += positionalBonus;
                
            } else {  // Black pieces - FIXED: Now properly symmetric
                score -= value;
                int blackRow = row;  // No flipping for black - tables are symmetric
                
                switch (pieceType) {
                    case 'P': 
                        positionalBonus = pawnTable[blackRow][col];
                        break;
                    case 'N':
                        positionalBonus = knightTable[blackRow][col];
                        break;
                    case 'B':
                        positionalBonus = bishopTable[blackRow][col];
                        break;
                    case 'R':
                        positionalBonus = rookTable[blackRow][col];
                        // Rook on open file bonus
                        if (blackPawnsOnFile[col] == 0) {
                            positionalBonus += 15;
                        }
                        // Rook on 7th rank bonus (row 6 for black)
                        if (blackRow == 6) {
                            positionalBonus += 20;
                        }
                        break;
                    case 'Q':
                        // Keep queen safe in early game
                        if (!blackEndgame) {
                            positionalBonus = -abs(col - 3) - abs(blackRow - 3);
                        }
                        break;
                    case 'K':
                        if (blackEndgame) {
                            positionalBonus = kingTableEndgame[blackRow][col];
                        } else {
                            positionalBonus = kingTable[blackRow][col];
                        }
                        break;
                }
                score -= positionalBonus;  // Subtract for black
            }
        }
    }
    
    // Pawn structure evaluation (symmetric)
    for (int col = 0; col < MAX_BOARD_SIZE; col++) {
        // Doubled pawn penalty
        if (whitePawnsOnFile[col] > 1) score -= 10 * (whitePawnsOnFile[col] - 1);
        if (blackPawnsOnFile[col] > 1) score += 10 * (blackPawnsOnFile[col] - 1);
        
        // Isolated pawn penalty
        int whiteIsolated = (whitePawnsOnFile[col] > 0) &&
                           (col == 0 || whitePawnsOnFile[col-1] == 0) &&
                           (col == MAX_BOARD_SIZE-1 || whitePawnsOnFile[col+1] == 0);
        int blackIsolated = (blackPawnsOnFile[col] > 0) &&
                           (col == 0 || blackPawnsOnFile[col-1] == 0) &&
                           (col == MAX_BOARD_SIZE-1 || blackPawnsOnFile[col+1] == 0);
        
        if (whiteIsolated) score -= 15;
        if (blackIsolated) score += 15;
    }
    
    // Bishop pair bonuses (symmetric)
    if (whiteBishopCount >= 2) score += 50;
    if (blackBishopCount >= 2) score -= 50;
    
    // Center control bonus (symmetric)
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