#include <stdio.h>
#include <ctype.h>
#include <bot.h>
#include <board.h>
#include <moves.h>
#include <stdlib.h>
#include <gameState.h>
#include <time.h>
#include <string.h>

#include <transposition.h>
#include <evaluation.h>
#include <moveOrdering.h>
#include <search.h>

// ============================================================================
// CONFIGURATION
// ============================================================================

double BOT_TIME_LIMIT_SECONDS = 60.0;  // Time limit for bot to make a move

// Constants for magic numbers
#define MAX_BOARD_SIZE 8
#define MAX_MOVES 256
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