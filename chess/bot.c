#include <stdio.h>
#include <ctype.h>
#include <bot.h>
#include <board.h>
#include <moves.h>
#include <stdlib.h>
#include <gameState.h>
#include <time.h>
#include <string.h>

// ============================================================================
// CONFIGURATION
// ============================================================================

static double BOT_TIME_LIMIT_SECONDS = 10.0;  // Time limit for bot to make a move

// Constants for magic numbers
#define MAX_BOARD_SIZE 8
#define MAX_PIECE_TYPES 12
#define MAX_MOVES 256
#define INITIAL_SEED 12345
#define NODES_BETWEEN_TIME_CHECKS 1000
#define MATE_SCORE_THRESHOLD 10000
#define INITIAL_ALPHA -999999
#define INITIAL_BETA 999999

void setBotDepth(int depth) {
    // Keep this function for compatibility, but we now use time-based search
    // We'll use depth as a rough guide: depth 6 ≈ 5 seconds
    BOT_TIME_LIMIT_SECONDS = depth * 0.8;
}

// ============================================================================
// TRANSPOSITION TABLE
// ============================================================================

// Zobrist hashing for position identification
static unsigned long long zobristTable[MAX_BOARD_SIZE][MAX_BOARD_SIZE][MAX_PIECE_TYPES];
static int zobristInitialized = 0;

// Transposition table entry
typedef struct {
    unsigned long long hash;
    int depth;
    int score;
    int flag;  // 0 = exact, 1 = lower bound (alpha), 2 = upper bound (beta)
    Move bestMove;
} TTEntry;

#define TT_SIZE 1048576  // 1M entries (about 32MB)
#define TT_EXACT 0
#define TT_ALPHA 1
#define TT_BETA 2

static TTEntry* transpositionTable = NULL;
static int transpositionTableInitialized = 0;

// Initialize Zobrist random numbers
static void initZobrist() {
    if (zobristInitialized) return;
    
    srand(INITIAL_SEED);  // Fixed seed for consistency
    for (int row = 0; row < MAX_BOARD_SIZE; row++) {
        for (int col = 0; col < MAX_BOARD_SIZE; col++) {
            for (int piece = 0; piece < MAX_PIECE_TYPES; piece++) {
                zobristTable[row][col][piece] = 
                    ((unsigned long long)rand() << 32) | rand();
            }
        }
    }
    zobristInitialized = 1;
}

// Map piece character to index (0-11)
static int pieceToIndex(char piece) {
    const char* pieces = "PNBRQKpnbrqk";
    for (int i = 0; i < MAX_PIECE_TYPES; i++) {
        if (pieces[i] == piece) return i;
    }
    return -1;
}

// Compute hash for current board position
static unsigned long long computeHash(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE]) {
    unsigned long long hash = 0;
    for (int row = 0; row < MAX_BOARD_SIZE; row++) {
        for (int col = 0; col < MAX_BOARD_SIZE; col++) {
            if (!isEmpty(board[row][col])) {
                int idx = pieceToIndex(board[row][col]);
                if (idx >= 0) {
                    hash ^= zobristTable[row][col][idx];
                }
            }
        }
    }
    return hash;
}

// Initialize transposition table
static int initTranspositionTable() {
    if (transpositionTable == NULL) {
        transpositionTable = (TTEntry*)calloc(TT_SIZE, sizeof(TTEntry));
        if (transpositionTable == NULL) {
            return 0; // Allocation failed
        }
        transpositionTableInitialized = 1;
    }
    initZobrist();
    return 1; // Success
}

// Free transposition table
static void freeTranspositionTable() {
    if (transpositionTable != NULL) {
        free(transpositionTable);
        transpositionTable = NULL;
        transpositionTableInitialized = 0;
    }
}

// Probe transposition table
static TTEntry* probeTranspositionTable(unsigned long long hash) {
    if (!transpositionTableInitialized) return NULL;
    
    int index = hash % TT_SIZE;
    TTEntry* entry = &transpositionTable[index];
    if (entry->hash == hash) {
        return entry;
    }
    return NULL;
}

// Store in transposition table
static void storeTranspositionTable(unsigned long long hash, int depth, int score, 
                                    int flag, Move* bestMove) {
    if (!transpositionTableInitialized) return;
    
    int index = hash % TT_SIZE;
    TTEntry* entry = &transpositionTable[index];
    
    // Replace if this is a deeper search or empty slot
    if (entry->hash == 0 || entry->depth <= depth) {
        entry->hash = hash;
        entry->depth = depth;
        entry->score = score;
        entry->flag = flag;
        if (bestMove) {
            entry->bestMove = *bestMove;
        }
    }
}

// ============================================================================
// KILLER MOVES
// ============================================================================

#define MAX_DEPTH 64
#define KILLERS_PER_DEPTH 2

// Make killer moves thread-local by using static (already done)
static Move killerMoves[MAX_DEPTH][KILLERS_PER_DEPTH];

static void clearKillerMoves() {
    memset(killerMoves, 0, sizeof(killerMoves));
}

static void storeKillerMove(Move* move, int depth) {
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

static int isKillerMove(Move* move, int depth) {
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

// ============================================================================
// BOARD EVALUATION (UNCHANGED)
// ============================================================================

static int evaluatePosition(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE]) {
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
    
    // Count pieces
    int whiteMinorCount = 0, blackMinorCount = 0;
    int whiteMajorCount = 0, blackMajorCount = 0;
    
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
                        whiteMinorCount++;
                        break;
                    case 'B':
                        positionalBonus = bishopTable[flippedRow][col];
                        whiteMinorCount++;
                        break;
                    case 'R':
                        positionalBonus = (flippedRow == MAX_BOARD_SIZE - 1) ? 10 : 0;
                        whiteMajorCount++;
                        break;
                    case 'Q':
                        whiteMajorCount++;
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
                        blackMinorCount++;
                        break;
                    case 'B':
                        positionalBonus = bishopTable[row][col];
                        blackMinorCount++;
                        break;
                    case 'R':
                        positionalBonus = (row == 0) ? 10 : 0;
                        blackMajorCount++;
                        break;
                    case 'Q':
                        blackMajorCount++;
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
            
            // Bishop pair bonus
            if (toupper(piece) == 'B') {
                int bishopCount = 0;
                for (int r = 0; r < MAX_BOARD_SIZE; r++) {
                    for (int c = 0; c < MAX_BOARD_SIZE; c++) {
                        char p = board[r][c];
                        if (isWhitePiece(piece) && p == 'B') bishopCount++;
                        if (!isWhitePiece(piece) && p == 'b') bishopCount++;
                    }
                }
                if (bishopCount >= 2) {
                    if (isWhitePiece(piece)) score += 25;
                    else score -= 25;
                }
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

// ============================================================================
// MOVE ORDERING
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

// Improved move scoring for ordering
static int scoreMoveForOrdering(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE], Move* move, Move* hashMove, int depth) {
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
static void sortMoves(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE], Move* moves, int numMoves, Move* hashMove, int depth) {
    // Calculate scores
    int* scores = (int*)malloc(numMoves * sizeof(int));
    if (scores == NULL) return; // Allocation failed
    
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
    
    free(scores);
}

// ============================================================================
// MOVE MAKING/UNMAKING
// ============================================================================

static void makeMove(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE], Move* move, char* savedStart, char* savedEnd, 
                     char* savedCaptured, int* wasEnPassant) {
    *savedStart = board[move->startRow][move->startCol];
    *savedEnd = board[move->endRow][move->endCol];
    *wasEnPassant = 0;
    *savedCaptured = '.';
    
    // Handle en passant
    if (toupper(*savedStart) == 'P' && move->endCol != move->startCol && 
        isEmpty(board[move->endRow][move->endCol])) {
        *savedCaptured = board[move->startRow][move->endCol];
        board[move->startRow][move->endCol] = '.';
        *wasEnPassant = 1;
    }
    
    board[move->endRow][move->endCol] = *savedStart;
    board[move->startRow][move->startCol] = '.';
}

static void unmakeMove(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE], Move* move, char savedStart, char savedEnd, 
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
// QUIESCENCE SEARCH
// ============================================================================

// Search only captures to avoid horizon effect
static int quiescenceSearch(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE], GameState* state, int alpha, int beta, 
                            int maximizing, int* nodesEvaluated, clock_t startTime) {
    (*nodesEvaluated)++;
    
    // Check time limit
    double elapsed = (double)(clock() - startTime) / CLOCKS_PER_SEC;
    if (elapsed >= BOT_TIME_LIMIT_SECONDS) {
        return evaluatePosition(board);
    }
    
    // Stand pat - current position evaluation
    int standPat = evaluatePosition(board);
    
    if (maximizing) {
        if (standPat >= beta) return beta;
        if (standPat > alpha) alpha = standPat;
    } else {
        if (standPat <= alpha) return alpha;
        if (standPat < beta) beta = standPat;
    }
    
    // Generate all moves and filter for captures only
    Move moves[MAX_MOVES];
    int numMoves = generateAllLegalMoves(board, maximizing, moves, state);
    
    // Only search capture moves
    Move captures[MAX_MOVES];
    int numCaptures = 0;
    for (int i = 0; i < numMoves; i++) {
        if (!isEmpty(board[moves[i].endRow][moves[i].endCol])) {
            captures[numCaptures++] = moves[i];
        }
    }
    
    // If no captures, return stand-pat score
    if (numCaptures == 0) {
        return standPat;
    }
    
    // Sort captures by MVV-LVA
    sortMoves(board, captures, numCaptures, NULL, 0);
    
    // Search captures
    for (int i = 0; i < numCaptures; i++) {
        char savedStart, savedEnd, savedCaptured;
        int wasEnPassant;
        GameState savedState = *state;
        
        makeMove(board, &captures[i], &savedStart, &savedEnd, &savedCaptured, &wasEnPassant);
        updateEnPassant(state, &captures[i], savedStart);
        
        int score = quiescenceSearch(board, state, alpha, beta, !maximizing, 
                                    nodesEvaluated, startTime);
        
        unmakeMove(board, &captures[i], savedStart, savedEnd, savedCaptured, wasEnPassant);
        *state = savedState;
        
        if (maximizing) {
            if (score >= beta) return beta;
            if (score > alpha) alpha = score;
        } else {
            if (score <= alpha) return alpha;
            if (score < beta) beta = score;
        }
    }
    
    return maximizing ? alpha : beta;
}

// ============================================================================
// MINIMAX WITH ALPHA-BETA PRUNING + OPTIMIZATIONS
// ============================================================================

static int minimax(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE], GameState* state, int depth, int alpha, int beta, 
                   int maximizing, int* nodesEvaluated, unsigned long long hash,
                   clock_t startTime, int ply) {
    (*nodesEvaluated)++;
    
    // Check time limit every ~1000 nodes
    if ((*nodesEvaluated) % NODES_BETWEEN_TIME_CHECKS == 0) {
        double elapsed = (double)(clock() - startTime) / CLOCKS_PER_SEC;
        if (elapsed >= BOT_TIME_LIMIT_SECONDS) {
            return evaluatePosition(board);
        }
    }
    
    // Probe transposition table
    TTEntry* ttEntry = probeTranspositionTable(hash);
    Move* hashMove = NULL;
    if (ttEntry != NULL && ttEntry->depth >= depth) {
        // Use stored score if depth is sufficient
        if (ttEntry->flag == TT_EXACT) {
            return ttEntry->score;
        } else if (ttEntry->flag == TT_ALPHA && ttEntry->score <= alpha) {
            return alpha;
        } else if (ttEntry->flag == TT_BETA && ttEntry->score >= beta) {
            return beta;
        }
        hashMove = &ttEntry->bestMove;
    }
    
    // Base case: reached depth limit, switch to quiescence search
    if (depth == 0) {
        return quiescenceSearch(board, state, alpha, beta, maximizing, 
                               nodesEvaluated, startTime);
    }
    
    // Check for game over
    if (!hasAnyLegalMoves(board, maximizing, state)) {
        return evaluatePosition(board);
    }
    
    // Null move pruning (skip if in check or in endgame)
    // Not implemented to keep code simpler - moderate complexity for the gain
    
    Move moves[MAX_MOVES];
    int numMoves = generateAllLegalMoves(board, maximizing, moves, state);
    
    // Sort moves for better pruning
    sortMoves(board, moves, numMoves, hashMove, ply);
    
    Move bestMove = moves[0];
    int originalAlpha = alpha;
    
    if (maximizing) {
        int maxScore = INITIAL_ALPHA;
        
        for (int i = 0; i < numMoves; i++) {
            char savedStart, savedEnd, savedCaptured;
            int wasEnPassant;
            GameState savedState = *state;
            
            makeMove(board, &moves[i], &savedStart, &savedEnd, &savedCaptured, &wasEnPassant);
            updateEnPassant(state, &moves[i], savedStart);
            unsigned long long newHash = computeHash(board);
            
            int score;
            
            // Late Move Reduction (LMR) - search later moves at reduced depth
            if (i >= 4 && depth >= 3 && isEmpty(savedEnd) && !isKillerMove(&moves[i], ply)) {
                // Search at reduced depth first
                score = minimax(board, state, depth - 2, alpha, beta, 0, 
                               nodesEvaluated, newHash, startTime, ply + 1);
                
                // If it looks good, re-search at full depth
                if (score > alpha) {
                    score = minimax(board, state, depth - 1, alpha, beta, 0, 
                                   nodesEvaluated, newHash, startTime, ply + 1);
                }
            } else {
                // Normal full-depth search
                score = minimax(board, state, depth - 1, alpha, beta, 0, 
                               nodesEvaluated, newHash, startTime, ply + 1);
            }
            
            unmakeMove(board, &moves[i], savedStart, savedEnd, savedCaptured, wasEnPassant);
            *state = savedState;
            
            if (score > maxScore) {
                maxScore = score;
                bestMove = moves[i];
            }
            
            if (score > alpha) alpha = score;
            
            if (beta <= alpha) {
                // Beta cutoff - store killer move if not a capture
                if (isEmpty(savedEnd)) {
                    storeKillerMove(&moves[i], ply);
                }
                break;
            }
        }
        
        // Store in transposition table
        int flag = (maxScore <= originalAlpha) ? TT_ALPHA : 
                   (maxScore >= beta) ? TT_BETA : TT_EXACT;
        storeTranspositionTable(hash, depth, maxScore, flag, &bestMove);
        
        return maxScore;
        
    } else {
        int minScore = INITIAL_BETA;
        
        for (int i = 0; i < numMoves; i++) {
            char savedStart, savedEnd, savedCaptured;
            int wasEnPassant;
            GameState savedState = *state;
            
            makeMove(board, &moves[i], &savedStart, &savedEnd, &savedCaptured, &wasEnPassant);
            updateEnPassant(state, &moves[i], savedStart);
            unsigned long long newHash = computeHash(board);
            
            int score;
            
            // Late Move Reduction
            if (i >= 4 && depth >= 3 && isEmpty(savedEnd) && !isKillerMove(&moves[i], ply)) {
                score = minimax(board, state, depth - 2, alpha, beta, 1, 
                               nodesEvaluated, newHash, startTime, ply + 1);
                
                if (score < beta) {
                    score = minimax(board, state, depth - 1, alpha, beta, 1, 
                                   nodesEvaluated, newHash, startTime, ply + 1);
                }
            } else {
                score = minimax(board, state, depth - 1, alpha, beta, 1, 
                               nodesEvaluated, newHash, startTime, ply + 1);
            }
            
            unmakeMove(board, &moves[i], savedStart, savedEnd, savedCaptured, wasEnPassant);
            *state = savedState;
            
            if (score < minScore) {
                minScore = score;
                bestMove = moves[i];
            }
            
            if (score < beta) beta = score;
            
            if (beta <= alpha) {
                if (isEmpty(savedEnd)) {
                    storeKillerMove(&moves[i], ply);
                }
                break;
            }
        }
        
        int flag = (minScore <= originalAlpha) ? TT_ALPHA : 
                   (minScore >= beta) ? TT_BETA : TT_EXACT;
        storeTranspositionTable(hash, depth, minScore, flag, &bestMove);
        
        return minScore;
    }
}

// ============================================================================
// ITERATIVE DEEPENING + BOT MOVE SELECTION
// ============================================================================

void selectBotMove(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE], int whiteToMove, int* startRow, int* startCol, 
                   int* endRow, int* endCol, GameState* state) {
    
    // Initialize data structures with error checking
    if (!initTranspositionTable()) {
        // Fallback: use first legal move if transposition table allocation fails
        Move moves[MAX_MOVES];
        int numMoves = generateAllLegalMoves(board, whiteToMove, moves, state);
        if (numMoves > 0) {
            *startRow = moves[0].startRow;
            *startCol = moves[0].startCol;
            *endRow = moves[0].endRow;
            *endCol = moves[0].endCol;
        } else {
            *startRow = -1;
            *startCol = -1;
            *endRow = -1;
            *endCol = -1;
        }
        return;
    }
    
    clearKillerMoves();
    
    Move moves[MAX_MOVES];
    int numMoves = generateAllLegalMoves(board, whiteToMove, moves, state);
    
    if (numMoves == 0) {
        *startRow = -1;
        *startCol = -1;
        *endRow = -1;
        *endCol = -1;
        freeTranspositionTable(); // Clean up before returning
        return;
    }
    
    Move bestMove = moves[0];
    int bestScore = whiteToMove ? INITIAL_ALPHA : INITIAL_BETA;
    int totalNodesEvaluated = 0;
    int depthReached = 0;
    
    clock_t startTime = clock();
    
    printf("\n=== Bot Thinking ===\n");
    printf("Time limit: %.1f seconds\n", BOT_TIME_LIMIT_SECONDS);
    
    // Iterative deepening: search depth 1, then 2, then 3, etc.
    for (int currentDepth = 1; currentDepth <= 50; currentDepth++) {
        double elapsed = (double)(clock() - startTime) / CLOCKS_PER_SEC;
        
        // Stop if we're running out of time
        if (elapsed >= BOT_TIME_LIMIT_SECONDS * 0.95) {
            printf("Time limit approaching, stopping at depth %d\n", currentDepth - 1);
            break;
        }
        
        int depthNodesEvaluated = 0;
        int depthBestScore = whiteToMove ? INITIAL_ALPHA : INITIAL_BETA;
        Move depthBestMove = moves[0];
        
        // Get hash move from previous iteration
        unsigned long long currentHash = computeHash(board);
        TTEntry* ttEntry = probeTranspositionTable(currentHash);
        Move* hashMove = ttEntry ? &ttEntry->bestMove : NULL;
        
        // Sort moves using info from previous iteration
        sortMoves(board, moves, numMoves, hashMove, 0);
        
        // Search all moves at current depth
        for (int i = 0; i < numMoves; i++) {
            char savedStart, savedEnd, savedCaptured;
            int wasEnPassant;
            GameState savedState = *state;
            
            makeMove(board, &moves[i], &savedStart, &savedEnd, &savedCaptured, &wasEnPassant);
            updateEnPassant(state, &moves[i], savedStart);
            unsigned long long newHash = computeHash(board);
            
            int score = minimax(board, state, currentDepth - 1, INITIAL_ALPHA, INITIAL_BETA, 
                               !whiteToMove, &depthNodesEvaluated, newHash, startTime, 1);
            
            unmakeMove(board, &moves[i], savedStart, savedEnd, savedCaptured, wasEnPassant);
            *state = savedState;
            
            // Update best move for this depth
            int isBetter = whiteToMove ? (score > depthBestScore) : (score < depthBestScore);
            if (isBetter) {
                depthBestScore = score;
                depthBestMove = moves[i];
            }
            
            // Check if we ran out of time
            elapsed = (double)(clock() - startTime) / CLOCKS_PER_SEC;
            if (elapsed >= BOT_TIME_LIMIT_SECONDS) {
                printf("Time expired during depth %d search\n", currentDepth);
                goto time_expired;
            }
        }
        
        // Successfully completed this depth
        bestMove = depthBestMove;
        bestScore = depthBestScore;
        totalNodesEvaluated += depthNodesEvaluated;
        depthReached = currentDepth;
        
        elapsed = (double)(clock() - startTime) / CLOCKS_PER_SEC;
        printf("Depth %2d: score=%6d, nodes=%8d, time=%.2fs\n", 
               currentDepth, depthBestScore, depthNodesEvaluated, elapsed);
        
        // Early exit if we found a forced mate
        if (whiteToMove && depthBestScore > MATE_SCORE_THRESHOLD) {
            printf("Found winning line, stopping search\n");
            break;
        }
        if (!whiteToMove && depthBestScore < -MATE_SCORE_THRESHOLD) {
            printf("Found winning line, stopping search\n");
            break;
        }
    }
    
time_expired:
    
    double totalTime = (double)(clock() - startTime) / CLOCKS_PER_SEC;
    
    printf("\n=== Search Complete ===\n");
    printf("Maximum depth reached: %d\n", depthReached);
    printf("Total nodes evaluated: %d\n", totalNodesEvaluated);
    printf("Nodes per second: %.0f\n", totalNodesEvaluated / (totalTime > 0 ? totalTime : 0.001));
    printf("Total time: %.2f seconds\n", totalTime);
    printf("Best move score: %d\n", bestScore);
    printf("Selected move: %c%d -> %c%d\n", 
           'a' + bestMove.startCol, MAX_BOARD_SIZE - bestMove.startRow,
           'a' + bestMove.endCol, MAX_BOARD_SIZE - bestMove.endRow);
    printf("===================\n\n");
    
    *startRow = bestMove.startRow;
    *startCol = bestMove.startCol;
    *endRow = bestMove.endRow;
    *endCol = bestMove.endCol;
    
    // Free the transposition table before returning
    freeTranspositionTable();
}