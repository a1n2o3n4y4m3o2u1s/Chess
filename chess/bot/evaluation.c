#include <evaluation.h>
#include <ctype.h>
#include <stdlib.h>
#include <board.h> 
#include <gameState.h>
#include <bot.h>

int evaluatePosition(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE], GameState* state) {
    // Check for checkmate first
    if (!hasAnyLegalMoves(board, 1, state)) {
        if (isKingInCheck(board, 1, state)) {
            return -MATE_SCORE;  // Black wins
        }
    }
    if (!hasAnyLegalMoves(board, 0, state)) {
        if (isKingInCheck(board, 0, state)) {
            return MATE_SCORE;   // White wins
        }
    }
    
    int mg_score = 0;
    int eg_score = 0;
    
    // Material values (centipawns, from PeSTO) - REDUCED KING VALUE
    static const int mg_values[6] = {82, 337, 365, 477, 1025, 1000};  // P, N, B, R, Q, K (reduced from 20000)
    static const int eg_values[6] = {94, 281, 297, 512, 936, 1000};
    
    // Phase increments (P=0, N=1, B=1, R=2, Q=4, K=0)
    static const int phase_inc[6] = {0, 1, 1, 2, 4, 0};
    
    // PeSTO PSQT (midgame)
    static const int mg_pawn_table[8][8] = {
        {  0,   0,   0,   0,   0,   0,   0,   0},
        { 98, 134,  61,  95,  68, 126,  34, -11},
        { -6,   7,  26,  31,  65,  56,  25, -20},
        {-14,  13,   6,  21,  23,  12,  17, -23},
        {-27,  -2,  -5,  12,  17,   6,  10, -25},
        {-26,  -4,  -4, -10,   3,   3,  33, -12},
        {-35,  -1, -20, -23, -15,  24,  38, -22},
        {  0,   0,   0,   0,   0,   0,   0,   0}
    };
    
    static const int mg_knight_table[8][8] = {
        {-167, -89, -34, -49,  61, -97, -15, -107},
        { -73, -41,  72,  36,  23,  62,   7, -17},
        { -47,  60,  37,  65,  84, 129,  73,  44},
        {  -9,  17,  19,  53,  37,  69,  18,  22},
        { -13,   4,  16,  13,  28,  19,  21,  -8},
        { -23,  -9,  12,  10,  19,  17,  25, -16},
        { -29, -53, -12,  -3,  -1,  18, -14, -19},
        {-105, -21, -58, -33, -17, -28, -19, -23}
    };
    
    static const int mg_bishop_table[8][8] = {
        { -29,   4, -82, -37, -25, -42,   7,  -8},
        { -26,  16, -18, -13,  30,  59,  18, -47},
        { -16,  37,  43,  40,  35,  50,  37,  -2},
        {  -4,   5,  19,  50,  37,  37,  7,  -2},
        {  -6,  13,  13,  26,  34,  12,  10,   4},
        {   0,  15,  15,  15,  14,  27,  18,  10},
        {   4,  15,  16,   0,   7,  21,  33,   1},
        { -33,  -3, -14, -21, -13, -12, -39, -21}
    };
    
    static const int mg_rook_table[8][8] = {
        { 32,  42,  32,  51, 63,  9,  31,  43},
        { 27,  32,  58,  62, 80, 67,  26,  44},
        { -5,  19,  26,  36, 17, 45,  61,  16},
        {-24, -11,   7,  26, 24, 35,  -8, -20},
        {-36, -26, -12,  -1,  9, -7,   6, -23},
        {-45, -25, -16, -17,  3,  0,  -5, -33},
        {-44, -16, -20,  -9, -1, 11,  -6, -71},
        {-19, -13,   1,  17, 16,  7, -37, -26}
    };
    
    static const int mg_queen_table[8][8] = {
        {-28,   0,  29,  12,  59,  44,  43,  45},
        {-24, -39,  -5,   1, -16,  57,  28,  54},
        {-13, -17,   7,   8,  29,  56,  47,  57},
        {-27, -27, -16, -16,  -1,  17,  -2,   1},
        { -9, -26,  -9, -10,  -2,  -4,   3,  -3},
        {-14,   2, -11,  -2,  -5,   2,  14,   5},
        {-35,  -8,  11,   2,   8,  15,  -3,   1},
        { -1, -18,  -9,  10, -15, -25, -31, -50}
    };
    
    static const int mg_king_table[8][8] = {
        {-65,  23,  16, -15, -56, -34,   2,  13},
        { 29,  -1, -20,  -7,  -8,  -4, -38, -29},
        { -9,  24,   2, -16, -20,   6,  22, -22},
        {-17, -20, -12, -27, -30, -25, -14, -36},
        {-49,  -1, -27, -39, -46, -44, -33, -51},
        {-14, -14, -22, -46, -44, -30, -15, -27},
        {  1,   7,  -8, -64, -43, -16,   9,   8},
        {-15,  36,  12, -54,   8, -28,  24,  14}
    };
    
    // PeSTO PSQT (endgame)
    static const int eg_pawn_table[8][8] = {
        {  0,   0,   0,   0,   0,   0,   0,   0},
        {178, 173, 158, 134, 147, 132, 165, 187},
        { 94, 100,  85,  67,  56,  53,  82,  84},
        { 32,  24,  13,   5,  -2,   4,  17,  17},
        { 13,   9,  -3,  -7,  -7,  -8,   3,  -1},
        {  4,   7,  -6,   1,   0,  -5,  -1,  -8},
        { 13,   8,   8,  10,  13,   0,   2,  -7},
        {  0,   0,   0,   0,   0,   0,   0,   0}
    };
    
    static const int eg_knight_table[8][8] = {
        { -58, -38, -13, -28, -31, -27, -63, -99},
        { -25,  -8, -25,  -2,  -9, -25, -24, -52},
        { -24, -20,  10,   9,  -1,  -9, -19, -41},
        { -17,   3,  22,  22,  22,  11,   8, -18},
        { -18,  -6,  16,  25,  16,  17,  4, -18},
        { -23,  -3,  -1,  15,  10,  -3, -20, -22},
        { -42, -20, -10,  -5,  -2, -20, -23, -44},
        { -29, -51, -23, -15, -22, -18, -50, -64}
    };
    
    static const int eg_bishop_table[8][8] = {
        { -14, -21, -11,  -8,  -7,  -9, -17, -24},
        {  -8,  -4,   7, -12,  -3, -13,  -4, -14},
        {   2,  -8,   0,  -1,  -2,   6,   0,   4},
        {  -3,   9,  12,   9,  14,  10,   3,   2},
        {  -6,   3,  13,  19,   7,  10,  -3,  -9},
        { -12,  -3,   8,  10,  13,   3,  -7, -15},
        { -14, -18,  -7,  -1,   4,  -9, -15, -27},
        { -23,  -9, -23,  -5,  -9, -16,  -5, -17}
    };
    
    static const int eg_rook_table[8][8] = {
        {13, 10, 18, 15, 12,  12,   8,   5},
        {11, 13, 13, 11, -3,   3,   8,   3},
        { 7,  7,  7,  5,  4,  -3,  -5,  -3},
        { 4,  3, 13,  1,  2,   1,  -1,   2},
        { 3,  5,  8,  4, -5,  -6,  -8, -11},
        {-4,  0, -5, -1, -7, -12,  -8, -16},
        {-6, -6,  0,  2, -9,  -9, -11,  -3},
        {-9,  2,  3, -1, -5, -13,   4, -20}
    };
    
    static const int eg_queen_table[8][8] = {
        { -9, 22, 22, 27, 27, 19, 10, 20},
        {-17, 20, 32, 41, 58, 25, 30,  0},
        {-20,  6,  9, 49, 47, 35, 19,  9},
        {  3, 22, 24, 45, 57, 40, 57, 36},
        {-18, 28, 19, 47, 31, 34, 39, 23},
        {-16,-27, 15,  6,  9, 17, 10,  5},
        {-22,-23,-30,-16,-16,-23,-36,-32},
        {-33,-28,-22,-43, -5,-32,-20,-41}
    };
    
    static const int eg_king_table[8][8] = {
        {-74, -35, -18, -18, -11, 15,   4, -17},
        {-12,  17,  14,  17,  17, 38,  23,  11},
        { 10,  17,  23,  15,  20, 45,  44,  13},
        { -8,  22,  24,  27,  26, 33,  26,   3},
        {-18,  -4,  21,  24,  27, 23,   9, -11},
        {-19,  -3,  11,  21,  23, 16,   7,  -9},
        {-27, -11,   4,  13,  14,  4,  -5, -17},
        {-53, -34, -21, -11, -28, -14, -24, -43}
    };
    
    // Piece counts and positions
    int whiteMinorCount = 0, blackMinorCount = 0;
    int whiteMajorCount = 0, blackMajorCount = 0;
    int whiteBishopCount = 0, blackBishopCount = 0;
    int whitePawnCount = 0, blackPawnCount = 0;
    int whitePawnsOnFile[8] = {0};
    int blackPawnsOnFile[8] = {0};
    int whitePawnCounts[8] = {0};
    int blackPawnCounts[8] = {0};
    int whitePawnRows[8][9];
    int blackPawnRows[8][9];
    int whiteKingRow = -1, whiteKingCol = -1;
    int blackKingRow = -1, blackKingCol = -1;
    int phasePoints = 0;
    
    // First pass: count pieces, record positions, compute phase
    for (int row = 0; row < MAX_BOARD_SIZE; row++) {
        for (int col = 0; col < MAX_BOARD_SIZE; col++) {
            char piece = board[row][col];
            if (isEmpty(piece)) continue;
            
            char pieceType = toupper(piece);
            int pieceIndex;
            switch (pieceType) {
                case 'P': pieceIndex = 0; break;
                case 'N': pieceIndex = 1; break;
                case 'B': pieceIndex = 2; break;
                case 'R': pieceIndex = 3; break;
                case 'Q': pieceIndex = 4; break;
                case 'K': pieceIndex = 5; break;
                default: continue;
            }
            
            phasePoints += phase_inc[pieceIndex];
            
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
                        whitePawnRows[col][whitePawnCounts[col]++] = row;
                        break;
                    case 'K':
                        whiteKingRow = row;
                        whiteKingCol = col;
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
                        blackPawnRows[col][blackPawnCounts[col]++] = row;
                        break;
                    case 'K':
                        blackKingRow = row;
                        blackKingCol = col;
                        break;
                }
            }
        }
    }
    
    // Clamp phasePoints
    if (phasePoints > 24) phasePoints = 24;
    
    // Tapered phase (mid_factor = 256 * phasePoints / 24)
    int mid_factor = (phasePoints * 256 + 12) / 24;
    int end_factor = 256 - mid_factor;
    
    // Second pass: evaluate material and positions
    for (int row = 0; row < MAX_BOARD_SIZE; row++) {
        for (int col = 0; col < MAX_BOARD_SIZE; col++) {
            char piece = board[row][col];
            if (isEmpty(piece)) continue;
            
            char pieceType = toupper(piece);
            int pieceIndex;
            switch (pieceType) {
                case 'P': pieceIndex = 0; break;
                case 'N': pieceIndex = 1; break;
                case 'B': pieceIndex = 2; break;
                case 'R': pieceIndex = 3; break;
                case 'Q': pieceIndex = 4; break;
                case 'K': pieceIndex = 5; break;
                default: continue;
            }
            
            int mg_val = mg_values[pieceIndex];
            int eg_val = eg_values[pieceIndex];
            int mg_pos = 0, eg_pos = 0;
            int isWhite = isWhitePiece(piece);
            int tableRow = row;
            
            if (!isWhite) {
                tableRow = MAX_BOARD_SIZE - 1 - row;
            }
            
            switch (pieceType) {
                case 'P': 
                    mg_pos = mg_pawn_table[tableRow][col];
                    eg_pos = eg_pawn_table[tableRow][col];
                    // Passed pawn bonus (added to endgame)
                    int passed = 1;
                    int bonus = 10 + (isWhite ? (7 - row) : row) * 10;
                    if (isWhite) {
                        for (int i = 0; i < blackPawnCounts[col]; i++) {
                            if (blackPawnRows[col][i] < row) {
                                passed = 0;
                                break;
                            }
                        }
                    } else {
                        for (int i = 0; i < whitePawnCounts[col]; i++) {
                            if (whitePawnRows[col][i] > row) {
                                passed = 0;
                                break;
                            }
                        }
                    }
                    if (passed) {
                        eg_pos += bonus;
                    }
                    break;
                case 'N':
                    mg_pos = mg_knight_table[tableRow][col];
                    eg_pos = eg_knight_table[tableRow][col];
                    break;
                case 'B':
                    mg_pos = mg_bishop_table[tableRow][col];
                    eg_pos = eg_bishop_table[tableRow][col];
                    break;
                case 'R':
                    mg_pos = mg_rook_table[tableRow][col];
                    eg_pos = eg_rook_table[tableRow][col];
                    // Rook on open file bonus (to midgame)
                    int ownPawns = isWhite ? whitePawnsOnFile[col] : blackPawnsOnFile[col];
                    if (ownPawns == 0) {
                        mg_pos += 15;
                    }
                    // Rook on 7th rank bonus - FIXED: Use relative ranks
                    int rank = isWhite ? row : MAX_BOARD_SIZE - 1 - row;
                    int on7th = (rank == 1); // 7th rank relative to the side
                    if (on7th) {
                        mg_pos += 20;
                    }
                    break;
                case 'Q':
                    mg_pos = mg_queen_table[tableRow][col];
                    eg_pos = eg_queen_table[tableRow][col];
                    break;
                case 'K':
                    mg_pos = mg_king_table[tableRow][col];
                    eg_pos = eg_king_table[tableRow][col];
                    break;
            }
            
            if (isWhite) {
                mg_score += mg_val + mg_pos;
                eg_score += eg_val + eg_pos;
            } else {
                mg_score -= mg_val + mg_pos;
                eg_score -= eg_val + eg_pos;
            }
        }
    }
    
    // Pawn structure evaluation
    for (int col = 0; col < MAX_BOARD_SIZE; col++) {
        // Doubled pawn penalty
        if (whitePawnsOnFile[col] > 1) {
            int penalty = 10 * (whitePawnsOnFile[col] - 1);
            mg_score -= penalty;
            eg_score -= penalty;
        }
        if (blackPawnsOnFile[col] > 1) {
            int penalty = 10 * (blackPawnsOnFile[col] - 1);
            mg_score += penalty;
            eg_score += penalty;
        }
        
        // Isolated pawn penalty
        int whiteIsolated = (whitePawnsOnFile[col] > 0) &&
                            (col == 0 || whitePawnsOnFile[col-1] == 0) &&
                            (col == MAX_BOARD_SIZE-1 || whitePawnsOnFile[col+1] == 0);
        int blackIsolated = (blackPawnsOnFile[col] > 0) &&
                            (col == 0 || blackPawnsOnFile[col-1] == 0) &&
                            (col == MAX_BOARD_SIZE-1 || blackPawnsOnFile[col+1] == 0);
        
        if (whiteIsolated) {
            mg_score -= 15;
            eg_score -= 15;
        }
        if (blackIsolated) {
            mg_score += 15;
            eg_score += 15;
        }
    }
    
    // Bishop pair bonuses
    if (whiteBishopCount >= 2) {
        mg_score += 50;
        eg_score += 50;
    }
    if (blackBishopCount >= 2) {
        mg_score -= 50;
        eg_score -= 50;
    }
    
    // Center control bonus
    for (int row = 3; row <= 4; row++) {
        for (int col = 3; col <= 4; col++) {
            char piece = board[row][col];
            if (!isEmpty(piece)) {
                if (isWhitePiece(piece)) mg_score += 5;
                else mg_score -= 5;
            }
        }
    }
    
    // SYMMETRIC AND FAST King safety evaluation
    int whiteSafety = 0;
    if (whiteKingRow != -1) {
        // Check the 3 squares in front of the white king
        int frontRow = whiteKingRow - 1;
        if (frontRow >= 0) {
            for (int col = whiteKingCol - 1; col <= whiteKingCol + 1; col++) {
                if (col >= 0 && col < MAX_BOARD_SIZE) {
                    if (board[frontRow][col] == 'P') whiteSafety += 10;
                }
            }
        }
    }
    mg_score += whiteSafety;
    
    int blackSafety = 0;
    if (blackKingRow != -1) {
        // Check the 3 squares in front of the black king
        int frontRow = blackKingRow + 1;
        if (frontRow < MAX_BOARD_SIZE) {
            for (int col = blackKingCol - 1; col <= blackKingCol + 1; col++) {
                if (col >= 0 && col < MAX_BOARD_SIZE) {
                    if (board[frontRow][col] == 'p') blackSafety += 10;
                }
            }
        }
    }
    mg_score -= blackSafety;
    
    // Tapered score
    int score = (mg_score * mid_factor + eg_score * end_factor) / 256;
    
    return score;
}