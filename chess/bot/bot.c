#include <stdio.h>
#include <ctype.h>
#include "bot.h"
#include "../board.h"
#include "../moves.h"
#include <stdlib.h>
#include "../gameState.h"
#include <time.h>
#include <string.h>

#include "transposition.h"
#include "evaluation.h"
#include "moveOrdering.h"
#include "search.h"

// ============================================================================
// CONFIGURATION
// ============================================================================

double BOT_TIME_LIMIT_SECONDS = 0.2;

// Constants for magic numbers
#define MAX_BOARD_SIZE 8
#define MAX_MOVES 256
#define NODES_BETWEEN_TIME_CHECKS 1000
#define MATE_SCORE_THRESHOLD 90000  // CHANGED: Detect near-mate scores
#define INITIAL_ALPHA -999999
#define INITIAL_BETA 999999

void setBotDepth(int depth) {
    BOT_TIME_LIMIT_SECONDS = depth * 0.8;
}

// ============================================================================
// PROMOTION HANDLING
// ============================================================================

char chooseBestPromotionPiece(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE], GameState* state, 
                             int startRow, int startCol, int endRow, int endCol, 
                             int whiteToMove, clock_t startTime) {
    char piece = board[startRow][startCol];
    int isWhite = isWhitePiece(piece);
    
    char bestPromotion = 'Q';
    int bestScore = isWhite ? INITIAL_ALPHA : INITIAL_BETA;
    
    char promotionPieces[] = {'Q', 'R', 'B', 'N'};
    int numOptions = 4;
    
    for (int i = 0; i < numOptions; i++) {
        Move testMove = {startRow, startCol, endRow, endCol, promotionPieces[i]};
        
        char savedStart, savedEnd, savedCaptured;
        int wasEnPassant;
        GameState savedState = *state;
        int nodesEvaluated = 0;
        
        makeMove(board, &testMove, &savedStart, &savedEnd, &savedCaptured, &wasEnPassant, state);
        
        int score = evaluatePosition(board, state);
        
        // FIXED: Remove color bias in promotion evaluation
        // Only add bonuses based on piece type, not color
        if (promotionPieces[i] == 'Q') {
            score += 50;  // Queen is generally best
        } else if (promotionPieces[i] == 'N') {
            // Knight promotion can be good in specific tactical situations
            score += 5;
        }
        // No bonus for rook/bishop as they're usually worse than queen
        
        unmakeMove(board, &testMove, savedStart, savedEnd, savedCaptured, wasEnPassant, state);
        *state = savedState;
        
        if ((isWhite && score > bestScore) || (!isWhite && score < bestScore)) {
            bestScore = score;
            bestPromotion = promotionPieces[i];
        }
        
        double elapsed = (double)(clock() - startTime) / CLOCKS_PER_SEC;
        if (elapsed >= BOT_TIME_LIMIT_SECONDS * 0.1) {
            break;
        }
    }
    
    printf("Selected promotion: %c\n", bestPromotion);
    return bestPromotion;
}

// ============================================================================
// ITERATIVE DEEPENING + BOT MOVE SELECTION
// ============================================================================

void selectBotMove(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE], int whiteToMove, int* startRow, int* startCol, 
                   int* endRow, int* endCol, GameState* state, double thinkTime, int currentEval) {
    
    BOT_TIME_LIMIT_SECONDS = thinkTime;
    
    if (!initTranspositionTable()) {
        Move moves[MAX_MOVES];
        int numMoves = generateAllLegalMoves(board, whiteToMove, moves, state);
        if (numMoves > 0) {
            *startRow = moves[0].startRow;
            *startCol = moves[0].startCol;
            *endRow = moves[0].endRow;
            *endCol = moves[0].endCol;
            
            char piece = board[*startRow][*startCol];
            if (toupper(piece) == 'P' && (*endRow == 0 || *endRow == 7)) {
                printf("Fallback: promoting to queen\n");
            }
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
        freeTranspositionTable();
        return;
    }
    
    // ============================================================================
    // CRITICAL FIX: IMMEDIATE MATE DETECTION
    // ============================================================================
    printf("Checking for immediate mates...\n");
    for (int i = 0; i < numMoves; i++) {
        char savedStart, savedEnd, savedCaptured;
        int wasEnPassant;
        GameState savedState = *state;
        
        makeMove(board, &moves[i], &savedStart, &savedEnd, &savedCaptured, &wasEnPassant, state);
        
        // Check if this move gives immediate mate
        if (!hasAnyLegalMoves(board, !whiteToMove, state)) {
            printf("*** FORCED MATE FOUND! Playing mating move immediately ***\n");
            
            *startRow = moves[i].startRow;
            *startCol = moves[i].startCol;
            *endRow = moves[i].endRow;
            *endCol = moves[i].endCol;
            
            // Handle promotion in mating move
            char piece = board[moves[i].startRow][moves[i].startCol];
            if (toupper(piece) == 'P' && (moves[i].endRow == 0 || moves[i].endRow == 7)) {
                if (moves[i].promotionPiece == 0) {
                    printf("Mating promotion - defaulting to queen\n");
                } else {
                    printf("Mating promotion to %c\n", moves[i].promotionPiece);
                }
            }
            
            unmakeMove(board, &moves[i], savedStart, savedEnd, savedCaptured, wasEnPassant, state);
            *state = savedState;
            freeTranspositionTable();
            return;
        }
        
        unmakeMove(board, &moves[i], savedStart, savedEnd, savedCaptured, wasEnPassant, state);
        *state = savedState;
    }
    
    Move bestMove = moves[0];
    int bestScore = whiteToMove ? INITIAL_ALPHA : INITIAL_BETA;
    int totalNodesEvaluated = 0;
    int depthReached = 0;
    
    clock_t startTime = clock();
    clock_t lastDepthStartTime = startTime;
    double lastDepthDuration = 0.0;
    
    printf("\n=== Bot Thinking ===\n");
    printf("Allocated time: %.1f seconds\n", thinkTime);
    printf("Position eval: %d\n", currentEval);
    printf("Legal moves: %d\n", numMoves);
    
    for (int currentDepth = 1; currentDepth <= 50; currentDepth++) {
        double elapsed = (double)(clock() - startTime) / CLOCKS_PER_SEC;
        
        if (elapsed >= thinkTime * 0.95) {
            printf("Time limit approaching, stopping at depth %d\n", currentDepth - 1);
            break;
        }
        
        if (currentDepth > 2) {
            double timeRemaining = thinkTime - elapsed;
            if (lastDepthDuration > timeRemaining * 0.8) {
                printf("Last depth took %.2fs, only %.2fs remaining - not starting depth %d\n",
                       lastDepthDuration, timeRemaining, currentDepth);
                break;
            }
        }
        
        lastDepthStartTime = clock();
        int depthNodesEvaluated = 0;
        int depthBestScore = whiteToMove ? INITIAL_ALPHA : INITIAL_BETA;
        Move depthBestMove = moves[0];
        
        unsigned long long currentHash = computeHash(board);
        TTEntry* ttEntry = probeTranspositionTable(currentHash);
        Move* hashMove = ttEntry ? &ttEntry->bestMove : NULL;
        
        sortMoves(board, moves, numMoves, hashMove, 0);
        
        int completedDepth = 1;
        for (int i = 0; i < numMoves; i++) {
            char savedStart, savedEnd, savedCaptured;
            int wasEnPassant;
            GameState savedState = *state;
            
            makeMove(board, &moves[i], &savedStart, &savedEnd, &savedCaptured, &wasEnPassant, state);
            updateEnPassant(state, &moves[i], savedStart);
            unsigned long long newHash = computeHash(board);
            
            int score = minimax(board, state, currentDepth - 1, INITIAL_ALPHA, INITIAL_BETA, 
                               !whiteToMove, &depthNodesEvaluated, newHash, startTime, 1);
            
            unmakeMove(board, &moves[i], savedStart, savedEnd, savedCaptured, wasEnPassant, state);
            *state = savedState;
            
            int isBetter = whiteToMove ? (score > depthBestScore) : (score < depthBestScore);
            if (isBetter) {
                depthBestScore = score;
                depthBestMove = moves[i];
                
                // Early stopping for clear mates
                if (whiteToMove && score > MATE_SCORE_THRESHOLD) {
                    printf("Found winning line at depth %d, stopping search\n", currentDepth);
                    completedDepth = 0;
                    break;
                }
                if (!whiteToMove && score < -MATE_SCORE_THRESHOLD) {
                    printf("Found winning line at depth %d, stopping search\n", currentDepth);
                    completedDepth = 0;
                    break;
                }
            }
            
            elapsed = (double)(clock() - startTime) / CLOCKS_PER_SEC;
            if (elapsed >= thinkTime) {
                printf("Time expired during depth %d search (after move %d/%d)\n", 
                       currentDepth, i + 1, numMoves);
                completedDepth = 0;
                goto time_expired;
            }
        }
        
        if (completedDepth) {
            bestMove = depthBestMove;
            bestScore = depthBestScore;
            totalNodesEvaluated += depthNodesEvaluated;
            depthReached = currentDepth;
            
            lastDepthDuration = (double)(clock() - lastDepthStartTime) / CLOCKS_PER_SEC;
            elapsed = (double)(clock() - startTime) / CLOCKS_PER_SEC;
            
            char promotionInfo[32] = "";
            if (bestMove.promotionPiece != 0) {
                snprintf(promotionInfo, sizeof(promotionInfo), " (promote to %c)", bestMove.promotionPiece);
            }
            
            // FIXED: Changed %8%d to %8d in the printf below
            printf("Depth %2d: score=%6d, nodes=%8d, time=%.2fs%s\n", 
                   currentDepth, depthBestScore, depthNodesEvaluated, elapsed, promotionInfo);
            
            // Early stopping for clear winning positions
            if (whiteToMove && depthBestScore > MATE_SCORE_THRESHOLD) {
                printf("Found winning line, stopping search\n");
                break;
            }
            if (!whiteToMove && depthBestScore < -MATE_SCORE_THRESHOLD) {
                printf("Found winning line, stopping search\n");
                break;
            }
        }
    }

    double totalTime;
    
    time_expired:
    
    totalTime = (double)(clock() - startTime) / CLOCKS_PER_SEC;
    
    // PROMOTION HANDLING
    char piece = board[bestMove.startRow][bestMove.startCol];
    int isPromotion = (toupper(piece) == 'P') && 
                     ((isWhitePiece(piece) && bestMove.endRow == 0) || 
                      (!isWhitePiece(piece) && bestMove.endRow == 7));
    
    if (isPromotion && bestMove.promotionPiece == 0) {
        printf("Choosing best promotion piece...\n");
        bestMove.promotionPiece = chooseBestPromotionPiece(board, state, 
                                                          bestMove.startRow, bestMove.startCol,
                                                          bestMove.endRow, bestMove.endCol,
                                                          whiteToMove, startTime);
    }
    
    printf("\n=== Search Complete ===\n");
    printf("Maximum depth reached: %d\n", depthReached);
    printf("Total nodes evaluated: %d\n", totalNodesEvaluated);
    printf("Nodes per second: %.0f\n", totalNodesEvaluated / (totalTime > 0 ? totalTime : 0.001));
    printf("Total time: %.2f seconds\n", totalTime);
    printf("Best move score: %d\n", bestScore);
    
    if (bestMove.promotionPiece != 0) {
        printf("Selected move: %c%d -> %c%d (promote to %c)\n", 
               'a' + bestMove.startCol, MAX_BOARD_SIZE - bestMove.startRow,
               'a' + bestMove.endCol, MAX_BOARD_SIZE - bestMove.endRow,
               bestMove.promotionPiece);
    } else {
        printf("Selected move: %c%d -> %c%d\n", 
               'a' + bestMove.startCol, MAX_BOARD_SIZE - bestMove.startRow,
               'a' + bestMove.endCol, MAX_BOARD_SIZE - bestMove.endRow);
    }
    
    if (toupper(piece) == 'K' && abs(bestMove.endCol - bestMove.startCol) == 2) {
        if (bestMove.endCol > bestMove.startCol) {
            printf("Castling: kingside\n");
        } else {
            printf("Castling: queenside\n");
        }
    }
    
    printf("===================\n\n");
    
    *startRow = bestMove.startRow;
    *startCol = bestMove.startCol;
    *endRow = bestMove.endRow;
    *endCol = bestMove.endCol;
    
    freeTranspositionTable();
}

// CHANGED: Remove the hardcoded 5.0 seconds - let main.c handle the default
void getBotMove(char board[8][8], int whiteToMove, int* startRow, int* startCol, 
                int* endRow, int* endCol, GameState* state) {
    // Use a reasonable fallback, but main.c should provide the configured value
    selectBotMove(board, whiteToMove, startRow, startCol, endRow, endCol, state, 2.0, 0);
}