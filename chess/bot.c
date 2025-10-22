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

// ============================================================================
// BOARD EVALUATION - IMPROVED VERSION
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
    
    // Pawn structure bonuses (middle game)
    static const int pawnTable[8][8] = {
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
    static const int knightTable[8][8] = {
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
    static const int bishopTable[8][8] = {
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
    static const int kingTable[8][8] = {
        {-30,-40,-40,-50,-50,-40,-40,-30},
        {-30,-40,-40,-50,-50,-40,-40,-30},
        {-30,-40,-40,-50,-50,-40,-40,-30},
        {-30,-40,-40,-50,-50,-40,-40,-30},
        {-20,-30,-30,-40,-40,-30,-30,-20},
        {-10,-20,-20,-20,-20,-20,-20,-10},
        { 20, 20,  0,  0,  0,  0, 20, 20},
        { 20, 30, 10,  0,  0, 10, 30, 20}
    };
    
    // Count pieces for game phase detection
    int whiteMinorCount = 0, blackMinorCount = 0;
    int whiteMajorCount = 0, blackMajorCount = 0;
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            char piece = board[row][col];
            if (isEmpty(piece)) continue;
            
            int value = pieceValues[(int)piece];
            int positionalBonus = 0;
            
            // Add positional bonuses based on piece type and square
            if (isWhitePiece(piece)) {
                score += value;
                
                // Piece-square tables (flipped for white - they move upward)
                int flippedRow = 7 - row;  // Flip for white perspective
                
                switch (toupper(piece)) {
                    case 'P': 
                        positionalBonus = pawnTable[flippedRow][col];
                        break;
                    case 'N':
                        positionalBonus = knightTable[flippedRow][col];
                        whiteMinorCount++;
                        break;
                    case 'B':
                        positionalBonus = bishopTable[flippedRow][col];
                        whiteMinorCount++;
                        break;
                    case 'R':
                        positionalBonus = (flippedRow == 7) ? 10 : 0; // Rook on 7th rank
                        whiteMajorCount++;
                        break;
                    case 'Q':
                        whiteMajorCount++;
                        break;
                    case 'K':
                        // Use king safety table in middle game, encourage castling
                        if (whiteMinorCount + whiteMajorCount > 6) { // Middle game
                            positionalBonus = kingTable[flippedRow][col];
                        } else { // Endgame - centralize king
                            positionalBonus = -abs(col - 3) - abs(flippedRow - 3);
                        }
                        break;
                }
                score += positionalBonus;
                
            } else {
                score -= value;
                
                // Piece-square tables (from black's perspective - they move downward)
                switch (toupper(piece)) {
                    case 'P': 
                        positionalBonus = pawnTable[row][col];
                        break;
                    case 'N':
                        positionalBonus = knightTable[row][col];
                        blackMinorCount++;
                        break;
                    case 'B':
                        positionalBonus = bishopTable[row][col];
                        blackMinorCount++;
                        break;
                    case 'R':
                        positionalBonus = (row == 0) ? 10 : 0; // Rook on 7th rank (row 0 for black)
                        blackMajorCount++;
                        break;
                    case 'Q':
                        blackMajorCount++;
                        break;
                    case 'K':
                        if (blackMinorCount + blackMajorCount > 6) { // Middle game
                            positionalBonus = kingTable[row][col];
                        } else { // Endgame - centralize king
                            positionalBonus = -abs(col - 3) - abs(row - 3);
                        }
                        break;
                }
                score -= positionalBonus;
            }
            
            // Bonus for bishop pair
            if (toupper(piece) == 'B') {
                int bishopCount = 0;
                for (int r = 0; r < 8; r++) {
                    for (int c = 0; c < 8; c++) {
                        char p = board[r][c];
                        if (isWhitePiece(piece) && p == 'B') bishopCount++;
                        if (!isWhitePiece(piece) && p == 'b') bishopCount++;
                    }
                }
                if (bishopCount >= 2) {
                    if (isWhitePiece(piece)) score += 25; // Bishop pair bonus
                    else score -= 25;
                }
            }
        }
    }
    
    // Bonus for controlling center squares
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