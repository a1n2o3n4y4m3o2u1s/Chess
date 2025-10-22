#include <moveOrdering.h>
#include <stdlib.h>
#include <string.h>
#include <moves.h>

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
        return 1000000;
    }
    
    // 2. Captures using MVV-LVA (Most Valuable Victim - Least Valuable Attacker)
    char targetPiece = board[move->endRow][move->endCol];
    if (!isEmpty(targetPiece)) {
        char movingPiece = board[move->startRow][move->startCol];
        int victimValue = getCaptureValue(targetPiece);
        int attackerValue = getCaptureValue(movingPiece);
        score = 100000 + (victimValue * 100) - attackerValue;
        return score;
    }
    
    // 3. Killer moves
    if (isKillerMove(move, depth)) {
        return 10000;
    }
    
    // 4. Center control moves
    if ((move->endRow >= 3 && move->endRow <= 4) && 
        (move->endCol >= 3 && move->endCol <= 4)) {
        score = 100;
    }
    
    return score;
}

// Sort moves by score
void sortMoves(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE], Move* moves, int numMoves, Move* hashMove, int depth) {
    // Calculate scores
    int scores[MAX_MOVES];
    
    for (int i = 0; i < numMoves; i++) {
        scores[i] = scoreMoveForOrdering(board, &moves[i], hashMove, depth);
    }
    
    // Simple bubble sort (good enough for ~30-40 moves)
    for (int i = 0; i < numMoves - 1; i++) {
        for (int j = i + 1; j < numMoves; j++) {
            if (scores[j] > scores[i]) {
                // Swap moves
                Move tempMove = moves[i];
                moves[i] = moves[j];
                moves[j] = tempMove;
                
                // Swap scores
                int tempScore = scores[i];
                scores[i] = scores[j];
                scores[j] = tempScore;
            }
        }
    }
}