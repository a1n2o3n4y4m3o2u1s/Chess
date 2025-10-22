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
// MOVE ORDERING - SIMPLIFIED VERSION
// ============================================================================

static int getCaptureValue(char capturedPiece) {
    if (isEmpty(capturedPiece)) return 0;
    
    int values[256] = {0};
    values['P'] = 1; values['p'] = 1;
    values['N'] = 3; values['n'] = 3;
    values['B'] = 3; values['b'] = 3;
    values['R'] = 5; values['r'] = 5;
    values['Q'] = 9; values['q'] = 9;
    
    return values[(int)capturedPiece];
}

static int estimateMoveScore(char board[8][8], Move* move) {
    // Check if move is a capture
    char targetPiece = board[move->endRow][move->endCol];
    if (!isEmpty(targetPiece)) {
        // MVV-LVA: Most Valuable Victim - Least Valuable Attacker
        char movingPiece = board[move->startRow][move->startCol];
        int victimValue = getCaptureValue(targetPiece);
        int attackerValue = getCaptureValue(movingPiece);
        return 1000 + (victimValue * 10) - attackerValue;
    }
    
    // Check for pawn pushes to center in endgame, etc.
    // Simple heuristic: prefer moves that go to center
    int centerBonus = 0;
    if ((move->endRow >= 3 && move->endRow <= 4) && 
        (move->endCol >= 3 && move->endCol <= 4)) {
        centerBonus = 10;
    }
    
    return centerBonus;
}

static int compareMoves(const void* a, const void* b) {
    const Move* moveA = (const Move*)a;
    const Move* moveB = (const Move*)b;
    
    // This is just a placeholder - we'll sort moves in the main function
    // where we have access to the board
    (void)moveA;
    (void)moveB;
    return 0;
}

// ============================================================================
// MOVE MAKING/UNMAKING (ORIGINAL VERSION - PROVEN TO WORK)
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

// ============================================================================
// IMPROVED MINIMAX WITH MOVE ORDERING
// ============================================================================

static int minimax(char board[8][8], GameState* state, int depth, int alpha, int beta, 
                   int maximizing, int* nodesEvaluated) {
    (*nodesEvaluated)++;
    
    // Base case: leaf node
    if (depth == 0) {
        return evaluatePosition(board);
    }
    
    // Check for game over
    if (!hasAnyLegalMoves(board, maximizing, state)) {
        return evaluatePosition(board);
    }
    
    Move moves[256]; // Reduced from 4096 for safety
    int numMoves = generateAllLegalMoves(board, maximizing, moves, state);
    
    if (maximizing) {
        int maxScore = -999999;
        
        // Simple move ordering: try captures first based on current board state
        for (int i = 0; i < numMoves; i++) {
            char savedStart, savedEnd, savedCaptured;
            int wasEnPassant;
            GameState savedState = *state;
            
            makeMove(board, &moves[i], &savedStart, &savedEnd, &savedCaptured, &wasEnPassant);
            updateEnPassant(state, &moves[i], savedStart);
            
            int score = minimax(board, state, depth - 1, alpha, beta, 0, nodesEvaluated);
            
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
            
            int score = minimax(board, state, depth - 1, alpha, beta, 1, nodesEvaluated);
            
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
// IMPROVED BOT MOVE SELECTION WITH MOVE ORDERING
// ============================================================================

void selectBotMove(char board[8][8], int whiteToMove, int* startRow, int* startCol, 
                   int* endRow, int* endCol, GameState* state) {
    Move moves[256]; // Reduced from 4096 for safety
    int numMoves = generateAllLegalMoves(board, whiteToMove, moves, state);
    
    if (numMoves == 0) {
        *startRow = -1;
        *startCol = -1;
        *endRow = -1;
        *endCol = -1;
        return;
    }
    
    // Simple move ordering: try captures first
    int moveScores[256];
    for (int i = 0; i < numMoves; i++) {
        moveScores[i] = estimateMoveScore(board, &moves[i]);
    }
    
    // Bubble sort moves by score (simple, works for small arrays)
    for (int i = 0; i < numMoves - 1; i++) {
        for (int j = i + 1; j < numMoves; j++) {
            if (moveScores[j] > moveScores[i]) {
                // Swap moves
                Move tempMove = moves[i];
                moves[i] = moves[j];
                moves[j] = tempMove;
                
                // Swap scores
                int tempScore = moveScores[i];
                moveScores[i] = moveScores[j];
                moveScores[j] = tempScore;
            }
        }
    }
    
    int bestMoveIndex = 0;
    int bestScore = whiteToMove ? -999999 : 999999;
    int nodesEvaluated = 0;
    
    printf("Bot searching %d plies deep...\n", BOT_SEARCH_DEPTH);
    
    for (int i = 0; i < numMoves; i++) {
        char savedStart, savedEnd, savedCaptured;
        int wasEnPassant;
        GameState savedState = *state;
        
        makeMove(board, &moves[i], &savedStart, &savedEnd, &savedCaptured, &wasEnPassant);
        updateEnPassant(state, &moves[i], savedStart);
        
        int score = minimax(board, state, BOT_SEARCH_DEPTH - 1, -999999, 999999, !whiteToMove, &nodesEvaluated);
        
        unmakeMove(board, &moves[i], savedStart, savedEnd, savedCaptured, wasEnPassant);
        *state = savedState;
        
        // Check if this is the best move found
        int isBetter = whiteToMove ? (score > bestScore) : (score < bestScore);
        if (isBetter) {
            bestScore = score;
            bestMoveIndex = i;
        }
        
        // Early exit if we find a winning move (for white) or losing move (for black)
        if (whiteToMove && score > 5000) break;
        if (!whiteToMove && score < -5000) break;
    }
    
    printf("Best move score: %d (evaluated %d nodes)\n", bestScore, nodesEvaluated);
    
    *startRow = moves[bestMoveIndex].startRow;
    *startCol = moves[bestMoveIndex].startCol;
    *endRow = moves[bestMoveIndex].endRow;
    *endCol = moves[bestMoveIndex].endCol;
}