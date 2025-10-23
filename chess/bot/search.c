#include "search.h"
#include "bot.h"
#include "transposition.h"
#include "evaluation.h"
#include "moveOrdering.h"
#include <moves.h>  // Keep as < > if it's outside bot/, otherwise "moves.h"
#include <board.h>  // Keep as < > if it's outside bot/, otherwise "board.h"
#include <ctype.h>
#include <stdlib.h>

void makeMove(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE], Move* move, char* savedStart, char* savedEnd, 
              char* savedCaptured, int* wasEnPassant, GameState* state) {
    *savedStart = board[move->startRow][move->startCol];
    *savedEnd = board[move->endRow][move->endCol];
    *wasEnPassant = 0;
    *savedCaptured = '.';
    
    int isWhite = isWhitePiece(*savedStart);
    
    // Handle castling first
    if (toupper(*savedStart) == 'K' && abs(move->endCol - move->startCol) == 2) {
        // Kingside castling
        if (move->endCol > move->startCol) {
            // Move rook from h-file to f-file
            int rookStartCol = 7;
            int rookEndCol = move->endCol - 1;
            *savedCaptured = board[move->startRow][rookStartCol]; // Save rook
            board[move->startRow][rookEndCol] = board[move->startRow][rookStartCol];
            board[move->startRow][rookStartCol] = '.';
        } 
        // Queenside castling
        else {
            // Move rook from a-file to d-file
            int rookStartCol = 0;
            int rookEndCol = move->endCol + 1;
            *savedCaptured = board[move->startRow][rookStartCol]; // Save rook
            board[move->startRow][rookEndCol] = board[move->startRow][rookStartCol];
            board[move->startRow][rookStartCol] = '.';
        }
        
        // Update castling rights
        if (isWhite) {
            state->whiteKingsideCastle = 0;
            state->whiteQueensideCastle = 0;
        } else {
            state->blackKingsideCastle = 0;
            state->blackQueensideCastle = 0;
        }
    }
    
    // Handle en passant
    if (toupper(*savedStart) == 'P' && move->endCol != move->startCol && 
        isEmpty(board[move->endRow][move->endCol])) {
        // For en passant, the captured pawn is on the same row as start, but target column
        *savedCaptured = board[move->startRow][move->endCol];
        board[move->startRow][move->endCol] = '.';
        *wasEnPassant = 1;
    }
    
    // Handle promotion
    int isPromotion = (toupper(*savedStart) == 'P') && 
                     ((isWhite && move->endRow == 0) || (!isWhite && move->endRow == 7));
    
    if (isPromotion) {
        char promotionPiece = move->promotionPiece;
        if (promotionPiece == 0) {
            // Default to queen if no promotion piece specified (for search)
            promotionPiece = isWhite ? 'Q' : 'q';
        } else {
            // Ensure correct case for the promotion piece
            promotionPiece = isWhite ? toupper(promotionPiece) : tolower(promotionPiece);
        }
        board[move->endRow][move->endCol] = promotionPiece;
    } else {
        board[move->endRow][move->endCol] = *savedStart;
    }
    
    board[move->startRow][move->startCol] = '.';
    
    // Update castling rights if rook is captured
    if (toupper(*savedEnd) == 'R') {
        if (move->endRow == 7 && move->endCol == 7) state->whiteKingsideCastle = 0;
        if (move->endRow == 7 && move->endCol == 0) state->whiteQueensideCastle = 0;
        if (move->endRow == 0 && move->endCol == 7) state->blackKingsideCastle = 0;
        if (move->endRow == 0 && move->endCol == 0) state->blackQueensideCastle = 0;
    }
    
    // Update castling rights if rook moves (already handled for king above)
    if (toupper(*savedStart) == 'R') {
        if (move->startRow == 7 && move->startCol == 7) state->whiteKingsideCastle = 0;
        if (move->startRow == 7 && move->startCol == 0) state->whiteQueensideCastle = 0;
        if (move->startRow == 0 && move->startCol == 7) state->blackKingsideCastle = 0;
        if (move->startRow == 0 && move->startCol == 0) state->blackQueensideCastle = 0;
    }
    
    // Update castling rights if king moves (already handled in castling section, but handle normal king moves)
    if (toupper(*savedStart) == 'K' && abs(move->endCol - move->startCol) != 2) {
        // Normal king move (not castling)
        if (isWhite) {
            state->whiteKingsideCastle = 0;
            state->whiteQueensideCastle = 0;
        } else {
            state->blackKingsideCastle = 0;
            state->blackQueensideCastle = 0;
        }
    }
}

void unmakeMove(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE], Move* move, char savedStart, char savedEnd, 
                char savedCaptured, int wasEnPassant, GameState* state) {
    int isWhite = isWhitePiece(savedStart);
    
    // Handle castling restoration first
    if (toupper(savedStart) == 'K' && abs(move->endCol - move->startCol) == 2) {
        // Kingside castling restoration
        if (move->endCol > move->startCol) {
            // Restore rook from f-file to h-file
            int rookCurrentCol = move->endCol - 1;
            int rookOriginalCol = 7;
            board[move->startRow][rookOriginalCol] = board[move->startRow][rookCurrentCol];
            board[move->startRow][rookCurrentCol] = '.';
        } 
        // Queenside castling restoration
        else {
            // Restore rook from d-file to a-file
            int rookCurrentCol = move->endCol + 1;
            int rookOriginalCol = 0;
            board[move->startRow][rookOriginalCol] = board[move->startRow][rookCurrentCol];
            board[move->startRow][rookCurrentCol] = '.';
        }
    }
    
    // Restore the main pieces
    board[move->startRow][move->startCol] = savedStart;
    board[move->endRow][move->endCol] = savedEnd;
    
    // Handle en passant capture restoration
    if (wasEnPassant) {
        // For en passant, the captured pawn was on the start row, end column
        board[move->startRow][move->endCol] = savedCaptured;
    }
}

void updateEnPassant(GameState* state, Move* move, char piece) {
    state->enPassantCol = -1;
    if (toupper(piece) == 'P' && abs(move->endRow - move->startRow) == 2) {
        state->enPassantCol = move->endCol;
        state->enPassantRow = (move->startRow + move->endRow) / 2;
    }
}

// ============================================================================
// QUIESCENCE SEARCH (UPDATED FOR PROMOTION)
// ============================================================================

// Search only captures to avoid horizon effect
int quiescenceSearch(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE], GameState* state, int alpha, int beta, 
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
    
    // Only search capture moves and promotions (treat promotions as captures)
    Move captures[MAX_MOVES];
    int numCaptures = 0;
    for (int i = 0; i < numMoves; i++) {
        char target = board[moves[i].endRow][moves[i].endCol];
        char piece = board[moves[i].startRow][moves[i].startCol];
        int isCapture = !isEmpty(target);
        int isPromotion = (toupper(piece) == 'P') && 
                         ((isWhitePiece(piece) && moves[i].endRow == 0) || 
                          (!isWhitePiece(piece) && moves[i].endRow == 7));
        
        if (isCapture || isPromotion) {
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
        
        makeMove(board, &captures[i], &savedStart, &savedEnd, &savedCaptured, &wasEnPassant, state);
        updateEnPassant(state, &captures[i], savedStart);
        
        int score = quiescenceSearch(board, state, alpha, beta, !maximizing, 
                                    nodesEvaluated, startTime);
        
        unmakeMove(board, &captures[i], savedStart, savedEnd, savedCaptured, wasEnPassant, state);
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
// MINIMAX WITH ALPHA-BETA PRUNING + OPTIMIZATIONS (UPDATED)
// ============================================================================

int minimax(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE], GameState* state, int depth, int alpha, int beta, 
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
            
            makeMove(board, &moves[i], &savedStart, &savedEnd, &savedCaptured, &wasEnPassant, state);
            updateEnPassant(state, &moves[i], savedStart);
            unsigned long long newHash = computeHash(board);
            
            int score;
            
            // Late Move Reduction (LMR) - search later moves at reduced depth
            int reduction = 2;
            if (i >= 4 && depth >= 3 && isEmpty(savedEnd) && !isKillerMove(&moves[i], ply)) {
                int reducedDepth = depth - reduction;
                if (reducedDepth <= 0) reducedDepth = 1;
                // Search at reduced depth first
                score = minimax(board, state, reducedDepth, alpha, beta, 0, 
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
            
            unmakeMove(board, &moves[i], savedStart, savedEnd, savedCaptured, wasEnPassant, state);
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
            
            makeMove(board, &moves[i], &savedStart, &savedEnd, &savedCaptured, &wasEnPassant, state);
            updateEnPassant(state, &moves[i], savedStart);
            unsigned long long newHash = computeHash(board);
            
            int score;
            
            // Late Move Reduction
            int reduction = 2;
            if (i >= 4 && depth >= 3 && isEmpty(savedEnd) && !isKillerMove(&moves[i], ply)) {
                int reducedDepth = depth - reduction;
                if (reducedDepth <= 0) reducedDepth = 1;
                score = minimax(board, state, reducedDepth, alpha, beta, 1, 
                               nodesEvaluated, newHash, startTime, ply + 1);
                
                if (score < beta) {
                    score = minimax(board, state, depth - 1, alpha, beta, 1, 
                                   nodesEvaluated, newHash, startTime, ply + 1);
                }
            } else {
                score = minimax(board, state, depth - 1, alpha, beta, 1, 
                               nodesEvaluated, newHash, startTime, ply + 1);
            }
            
            unmakeMove(board, &moves[i], savedStart, savedEnd, savedCaptured, wasEnPassant, state);
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