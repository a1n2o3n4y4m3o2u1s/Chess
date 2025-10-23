#include <moveOrdering.h>
#include <stdlib.h>
#include <string.h>
#include <moves.h>
#include <board.h> 

#define SCORE_TT_MOVE 1000000
#define SCORE_CAPTURE_BASE 100000
#define SCORE_PROMOTION 90000
#define SCORE_KILLER 10000
#define SCORE_CASTLING 8000
#define SCORE_CENTER 100

// Killer moves
static Move killerMoves[MAX_DEPTH][KILLERS_PER_DEPTH];

void clearKillerMoves(void) {
    memset(killerMoves, 0, sizeof(killerMoves));
}

void storeKillerMove(Move* move, int depth) {
    if (depth >= MAX_DEPTH) return;
    
    // Don't store if it's already the first killer
    if (killerMoves[depth][0].startRow == move->startRow &&
        killerMoves[depth][0].startCol == move->startCol &&
        killerMoves[depth][0].endRow == move->endRow &&
        killerMoves[depth][0].endCol == move->endCol) {
        return;
    }
    
    // Shift and store
    killerMoves[depth][1] = killerMoves[depth][0];
    killerMoves[depth][0] = *move;
}

int isKillerMove(Move* move, int depth) {
    if (depth >= MAX_DEPTH) return 0;
    
    for (int i = 0; i < KILLERS_PER_DEPTH; i++) {
        if (killerMoves[depth][i].startRow == move->startRow &&
            killerMoves[depth][i].startCol == move->startCol &&
            killerMoves[depth][i].endRow == move->endRow &&
            killerMoves[depth][i].endCol == move->endCol) {
            return 1;
        }
    }
    return 0;
}

int getCaptureValue(char capturedPiece) {
    if (isEmpty(capturedPiece)) return 0;
    
    int values[256] = {0};
    values['P'] = 1; values['p'] = 1;
    values['N'] = 3; values['n'] = 3;
    values['B'] = 3; values['b'] = 3;
    values['R'] = 5; values['r'] = 5;
    values['Q'] = 9; values['q'] = 9;
    values['K'] = 100; values['k'] = 100;
    
    return values[(int)capturedPiece];
}

// Improved move scoring for ordering
int scoreMoveForOrdering(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE], Move* move, Move* hashMove, int depth) {
    int score = 0;
    
    // 1. Hash move gets highest priority (from transposition table)
    if (hashMove && 
        move->startRow == hashMove->startRow &&
        move->startCol == hashMove->startCol &&
        move->endRow == hashMove->endRow &&
        move->endCol == hashMove->endCol) {
        return SCORE_TT_MOVE;
    }
    
    char movingPiece = board[move->startRow][move->startCol];
    char targetPiece = board[move->endRow][move->endCol];
    
    // 2. Captures using MVV-LVA (Most Valuable Victim - Least Valuable Attacker)
    if (!isEmpty(targetPiece)) {
        int victimValue = getCaptureValue(targetPiece);
        int attackerValue = getCaptureValue(movingPiece);
        score = SCORE_CAPTURE_BASE + (victimValue * 100) - attackerValue;
        return score;
    }
    
    // Quiet moves below
    
    // 3. Promotions (assuming to queen, detect by pawn to back rank)
    // Assuming row 0 is rank 8 (black's back), row 7 is rank 1 (white's back)
    if ((movingPiece == 'P' && move->endRow == 0) || 
        (movingPiece == 'p' && move->endRow == MAX_BOARD_SIZE - 1)) {
        return SCORE_PROMOTION;
    }
    
    // 4. Killer moves
    if (isKillerMove(move, depth)) {
        return SCORE_KILLER;
    }
    
    // 5. Castling
    if ((movingPiece == 'K' || movingPiece == 'k') &&
        abs(move->endCol - move->startCol) == 2 &&
        move->startRow == move->endRow) {
        score = SCORE_CASTLING;
    }
    
    // 6. Center control moves
    int centerStart = (MAX_BOARD_SIZE / 2) - 1;
    int centerEnd = centerStart + 1;
    if ((move->endRow >= centerStart && move->endRow <= centerEnd) && 
        (move->endCol >= centerStart && move->endCol <= centerEnd)) {
        score += SCORE_CENTER;
    }
    
    return score;
}

typedef struct {
    Move move;
    int score;
} ScoredMove;

static int compareScoredMoves(const void* a, const void* b) {
    return ((ScoredMove*)b)->score - ((ScoredMove*)a)->score;  // Descending order
}

// Sort moves by score
void sortMoves(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE], Move* moves, int numMoves, Move* hashMove, int depth) {
    if (numMoves <= 1) return;
    
    ScoredMove scoredMoves[MAX_MOVES];
    
    for (int i = 0; i < numMoves; i++) {
        scoredMoves[i].move = moves[i];
        scoredMoves[i].score = scoreMoveForOrdering(board, &moves[i], hashMove, depth);
    }
    
    qsort(scoredMoves, numMoves, sizeof(ScoredMove), compareScoredMoves);
    
    for (int i = 0; i < numMoves; i++) {
        moves[i] = scoredMoves[i].move;
    }
}