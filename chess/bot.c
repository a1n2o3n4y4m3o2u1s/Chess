#include <stdio.h>
#include <ctype.h>
#include <bot.h>
#include <board.h>
#include <moves.h>
#include <stdlib.h>
#include <gameState.h>

static int BOT_SEARCH_DEPTH = 6;

void setBotDepth(int depth) {
    BOT_SEARCH_DEPTH = depth;
}

// ============================================================================
// BOARD EVALUATION
// ============================================================================

static int evaluatePosition(char board[8][8]) {
    int score = 0;
    
    // Material values (centipawns)
    int pieceValues[256] = {0};
    pieceValues['P'] = 100;   pieceValues['p'] = 100;
    pieceValues['N'] = 320;   pieceValues['n'] = 320;
    pieceValues['B'] = 330;   pieceValues['b'] = 330;
    pieceValues['R'] = 500;   pieceValues['r'] = 500;
    pieceValues['Q'] = 900;   pieceValues['q'] = 900;
    pieceValues['K'] = 20000; pieceValues['k'] = 20000;
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            char piece = board[row][col];
            if (isEmpty(piece)) continue;
            
            int value = pieceValues[(int)piece];
            if (isWhitePiece(piece)) {
                score += value;
            } else {
                score -= value;
            }
        }
    }
    
    return score;
}

// ============================================================================
// MINIMAX WITH ALPHA-BETA PRUNING
// ============================================================================

static void makeMove(char board[8][8], Move* move, char* savedStart, char* savedEnd, 
                     char* savedCaptured, int* wasEnPassant) {
    *savedStart = board[move->startRow][move->startCol];
    *savedEnd = board[move->endRow][move->endCol];
    *wasEnPassant = 0;
    *savedCaptured = '.';
    
    // Handle en passant capture
    if (toupper(*savedStart) == 'P' && move->endCol != move->startCol && 
        isEmpty(board[move->endRow][move->endCol])) {
        *savedCaptured = board[move->startRow][move->endCol];
        board[move->startRow][move->endCol] = '.';
        *wasEnPassant = 1;
    }
    
    board[move->endRow][move->endCol] = *savedStart;
    board[move->startRow][move->startCol] = '.';
}

static void unmakeMove(char board[8][8], Move* move, char savedStart, char savedEnd, 
                       char savedCaptured, int wasEnPassant) {
    board[move->startRow][move->startCol] = savedStart;
    board[move->endRow][move->endCol] = savedEnd;
    if (wasEnPassant) {
        board[move->startRow][move->endCol] = savedCaptured;
    }
}

static void updateEnPassant(GameState* state, Move* move, char piece) {
    state->enPassantCol = -1;
    if (toupper(piece) == 'P' && abs(move->endRow - move->startRow) == 2) {
        state->enPassantCol = move->endCol;
        state->enPassantRow = (move->startRow + move->endRow) / 2;
    }
}

static int minimax(char board[8][8], GameState* state, int depth, int alpha, int beta, 
                   int maximizing) {
    // Base case: leaf node or game over
    if (depth == 0 || !hasAnyLegalMoves(board, maximizing, state)) {
        return evaluatePosition(board);
    }
    
    Move moves[4096];
    int numMoves = generateAllLegalMoves(board, maximizing, moves, state);
    
    if (maximizing) {
        int maxScore = -999999;
        
        for (int i = 0; i < numMoves; i++) {
            char savedStart, savedEnd, savedCaptured;
            int wasEnPassant;
            GameState savedState = *state;
            
            makeMove(board, &moves[i], &savedStart, &savedEnd, &savedCaptured, &wasEnPassant);
            updateEnPassant(state, &moves[i], savedStart);
            
            int score = minimax(board, state, depth - 1, alpha, beta, 0);
            
            unmakeMove(board, &moves[i], savedStart, savedEnd, savedCaptured, wasEnPassant);
            *state = savedState;
            
            if (score > maxScore) maxScore = score;
            if (score > alpha) alpha = score;
            if (beta <= alpha) break;  // Beta cutoff
        }
        return maxScore;
        
    } else {
        int minScore = 999999;
        
        for (int i = 0; i < numMoves; i++) {
            char savedStart, savedEnd, savedCaptured;
            int wasEnPassant;
            GameState savedState = *state;
            
            makeMove(board, &moves[i], &savedStart, &savedEnd, &savedCaptured, &wasEnPassant);
            updateEnPassant(state, &moves[i], savedStart);
            
            int score = minimax(board, state, depth - 1, alpha, beta, 1);
            
            unmakeMove(board, &moves[i], savedStart, savedEnd, savedCaptured, wasEnPassant);
            *state = savedState;
            
            if (score < minScore) minScore = score;
            if (score < beta) beta = score;
            if (beta <= alpha) break;  // Alpha cutoff
        }
        return minScore;
    }
}

// ============================================================================
// BOT MOVE SELECTION
// ============================================================================

void selectBotMove(char board[8][8], int whiteToMove, int* startRow, int* startCol, 
                   int* endRow, int* endCol, GameState* state) {
    Move moves[4096];
    int numMoves = generateAllLegalMoves(board, whiteToMove, moves, state);
    
    if (numMoves == 0) {
        *startRow = -1;
        *startCol = -1;
        *endRow = -1;
        *endCol = -1;
        return;
    }
    
    int bestMoveIndex = 0;
    int bestScore = whiteToMove ? -999999 : 999999;
    
    printf("Searching %d moves deep...\n", BOT_SEARCH_DEPTH / 2);
    
    for (int i = 0; i < numMoves; i++) {
        char savedStart, savedEnd, savedCaptured;
        int wasEnPassant;
        GameState savedState = *state;
        
        makeMove(board, &moves[i], &savedStart, &savedEnd, &savedCaptured, &wasEnPassant);
        updateEnPassant(state, &moves[i], savedStart);
        
        int score = minimax(board, state, BOT_SEARCH_DEPTH - 1, -999999, 999999, !whiteToMove);
        
        unmakeMove(board, &moves[i], savedStart, savedEnd, savedCaptured, wasEnPassant);
        *state = savedState;
        
        // Check if this is the best move found
        int isBetter = whiteToMove ? (score > bestScore) : (score < bestScore);
        if (isBetter) {
            bestScore = score;
            bestMoveIndex = i;
        }
    }
    
    printf("Best move score: %d\n", bestScore);
    
    *startRow = moves[bestMoveIndex].startRow;
    *startCol = moves[bestMoveIndex].startCol;
    *endRow = moves[bestMoveIndex].endRow;
    *endCol = moves[bestMoveIndex].endCol;
}