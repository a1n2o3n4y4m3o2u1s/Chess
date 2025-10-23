#include <stdlib.h>
#include <ctype.h>
#include <moves.h>
#include <board.h>
#include <gameState.h>

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

static int isCorrectColorMoving(char board[8][8], int row, int col, int whiteToMove) {
    char piece = board[row][col];
    if (whiteToMove) {
        return isWhitePiece(piece);
    } else {
        return isBlackPiece(piece);
    }
}

static int isNotCapturingSameColor(char board[8][8], int row, int col, int whiteToMove) {
    char targetPiece = board[row][col];
    if (isEmpty(targetPiece)) return 1;
    
    if (whiteToMove) {
        return !isWhitePiece(targetPiece);
    } else {
        return !isBlackPiece(targetPiece);
    }
}

static int isPathClear(char board[8][8], int startRow, int startCol, int endRow, int endCol, 
                       int rowStep, int colStep) {
    int currentRow = startRow + rowStep;
    int currentCol = startCol + colStep;
    
    while (currentRow != endRow || currentCol != endCol) {
        if (!isEmpty(board[currentRow][currentCol])) {
            return 0;  // Path blocked
        }
        currentRow += rowStep;
        currentCol += colStep;
    }
    return 1;  // Path clear
}

// ============================================================================
// MOVE GENERATION OFFSETS
// ============================================================================

static const int knightOffsets[8][2] = {
    {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
    {1, -2}, {1, 2}, {2, -1}, {2, 1}
};

static const int bishopOffsets[4][2] = {
    {-1, -1}, {-1, 1}, {1, -1}, {1, 1}
};

static const int rookOffsets[4][2] = {
    {-1, 0}, {1, 0}, {0, -1}, {0, 1}
};

static const int queenKingOffsets[8][2] = {
    {-1, -1}, {-1, 0}, {-1, 1}, {0, -1},
    {0, 1}, {1, -1}, {1, 0}, {1, 1}
};

// ============================================================================
// PROMOTION HELPER FUNCTION
// ============================================================================

int isLegalMoveWithPromotion(char board[8][8], int startRow, int startCol, int endRow, int endCol, 
                            int whiteToMove, GameState* state, char promotionPiece) {
    // Check basic validity
    if (!isCorrectColorMoving(board, startRow, startCol, whiteToMove)) return 0;
    if (!isNotCapturingSameColor(board, endRow, endCol, whiteToMove)) return 0;
    
    // Check piece-specific rules
    if (!canPieceMoveTo(board, startRow, startCol, endRow, endCol, state)) return 0;
    
    // For pawn promotion, verify it's a valid promotion square and piece
    char piece = board[startRow][startCol];
    if (toupper(piece) == 'P' && (endRow == 0 || endRow == 7)) {
        char validPieces[] = {'Q', 'R', 'B', 'N'};
        int isValidPromotion = 0;
        for (int i = 0; i < 4; i++) {
            if (toupper(promotionPiece) == validPieces[i]) {
                isValidPromotion = 1;
                break;
            }
        }
        if (!isValidPromotion) return 0;
    }
    
    // Move cannot leave own king in check
    if (doesMovePutKingInCheck(board, startRow, startCol, endRow, endCol, whiteToMove, state)) {
        return 0;
    }
    
    return 1;
}

// ============================================================================
// CASTLING MOVE GENERATION
// ============================================================================

// ============================================================================
// CORRECTED CASTLING MOVE GENERATION
// ============================================================================

static int generateCastlingMoves(char board[8][8], int row, int col, Move moves[], int count, int whiteToMove, GameState* state) {
    // Only kings can castle
    if (toupper(board[row][col]) != 'K') return count;
    
    int isWhite = isWhitePiece(board[row][col]);
    
    // Kingside castling
    if ((isWhite && state->whiteKingsideCastle) || (!isWhite && state->blackKingsideCastle)) {
        int rookCol = 7; // h-file
        int kingTargetCol = col + 2;
        int rookTargetCol = col + 1;
        
        // Check if path is clear
        if (isEmpty(board[row][col+1]) && isEmpty(board[row][col+2])) {
            // Check if rook is in correct position
            char expectedRook = isWhite ? 'R' : 'r';
            if (board[row][rookCol] == expectedRook) {
                // Check if squares are not attacked
                if (!isSquareAttacked(board, row, col, !whiteToMove, state) &&
                    !isSquareAttacked(board, row, col+1, !whiteToMove, state) &&
                    !isSquareAttacked(board, row, col+2, !whiteToMove, state)) {
                    moves[count++] = (Move){row, col, row, kingTargetCol, 0};
                }
            }
        }
    }
    
    // Queenside castling
    if ((isWhite && state->whiteQueensideCastle) || (!isWhite && state->blackQueensideCastle)) {
        int rookCol = 0; // a-file
        int kingTargetCol = col - 2;
        int rookTargetCol = col - 1;
        
        // Check if path is clear (queenside has 3 squares between rook and king)
        if (isEmpty(board[row][col-1]) && isEmpty(board[row][col-2]) && isEmpty(board[row][col-3])) {
            // Check if rook is in correct position
            char expectedRook = isWhite ? 'R' : 'r';
            if (board[row][rookCol] == expectedRook) {
                // Check if squares are not attacked (don't need to check b1/b8 for white/black)
                if (!isSquareAttacked(board, row, col, !whiteToMove, state) &&
                    !isSquareAttacked(board, row, col-1, !whiteToMove, state) &&
                    !isSquareAttacked(board, row, col-2, !whiteToMove, state)) {
                    moves[count++] = (Move){row, col, row, kingTargetCol, 0};
                }
            }
        }
    }
    
    return count;
}

// ============================================================================
// PIECE MOVEMENT RULES (KEEP ORIGINAL SIGNATURES)
// ============================================================================

int isValidPawnMove(char board[8][8], int startRow, int startCol, int endRow, int endCol, 
                    GameState* state) {
    char piece = board[startRow][startCol];
    int isWhite = isWhitePiece(piece);
    int direction = isWhite ? -1 : 1;  // White moves up, black moves down
    
    int rowDiff = endRow - startRow;
    int colDiff = abs(endCol - startCol);
    
    // Forward move (no capture)
    if (colDiff == 0) {
        if (!isEmpty(board[endRow][endCol])) return 0;
        
        // Single step forward
        if (rowDiff == direction) return 1;
        
        // Double step from starting position
        if (rowDiff == 2 * direction) {
            int startRank = isWhite ? 6 : 1;
            if (startRow != startRank) return 0;
            
            int middleRow = startRow + direction;
            if (!isEmpty(board[middleRow][startCol])) return 0;
            return 1;
        }
        return 0;
    }
    
    // Diagonal capture
    if (colDiff == 1 && rowDiff == direction) {
        // Normal capture
        if (!isEmpty(board[endRow][endCol])) return 1;
        
        // En passant
        if (state->enPassantCol == endCol && state->enPassantRow == endRow) {
            return 1;
        }
    }
    
    return 0;
}

int isValidKnightMove(int startRow, int startCol, int endRow, int endCol) {
    int rowDiff = abs(endRow - startRow);
    int colDiff = abs(endCol - startCol);
    
    // Check knight movement pattern
    return (rowDiff == 2 && colDiff == 1) || (rowDiff == 1 && colDiff == 2);
}

int isValidBishopMove(char board[8][8], int startRow, int startCol, int endRow, int endCol) {
    int rowDiff = abs(endRow - startRow);
    int colDiff = abs(endCol - startCol);
    
    // Must move diagonally
    if (rowDiff != colDiff || rowDiff == 0) return 0;
    
    int rowStep = (endRow > startRow) ? 1 : -1;
    int colStep = (endCol > startCol) ? 1 : -1;
    
    return isPathClear(board, startRow, startCol, endRow, endCol, rowStep, colStep);
}

int isValidRookMove(char board[8][8], int startRow, int startCol, int endRow, int endCol) {
    int rowDiff = abs(endRow - startRow);
    int colDiff = abs(endCol - startCol);
    
    // Must move straight (horizontal or vertical)
    int isHorizontal = (rowDiff == 0 && colDiff > 0);
    int isVertical = (colDiff == 0 && rowDiff > 0);
    if (!isHorizontal && !isVertical) return 0;
    
    int rowStep = (rowDiff == 0) ? 0 : (endRow > startRow ? 1 : -1);
    int colStep = (colDiff == 0) ? 0 : (endCol > startCol ? 1 : -1);
    
    return isPathClear(board, startRow, startCol, endRow, endCol, rowStep, colStep);
}

int isValidQueenMove(char board[8][8], int startRow, int startCol, int endRow, int endCol) {
    return isValidBishopMove(board, startRow, startCol, endRow, endCol) ||
           isValidRookMove(board, startRow, startCol, endRow, endCol);
}

int isValidKingMove(char board[8][8], int startRow, int startCol, int endRow, int endCol, 
                    GameState* state) {
    int rowDiff = abs(endRow - startRow);
    int colDiff = abs(endCol - startCol);
    
    // Normal king move (one square)
    if (rowDiff <= 1 && colDiff <= 1) return 1;
    
    // Castling attempt (king moves 2 squares horizontally)
    if (rowDiff == 0 && colDiff == 2) {
        int isWhite = isWhitePiece(board[startRow][startCol]);
        
        // White castling
        if (isWhite && startRow == 7 && startCol == 4) {
            if (endCol == 6 && state->whiteKingsideCastle) {
                if (isEmpty(board[7][5]) && isEmpty(board[7][6]) && board[7][7] == 'R') {
                    if (isSquareAttacked(board, 7, 4, 0, state) ||
                        isSquareAttacked(board, 7, 5, 0, state) ||
                        isSquareAttacked(board, 7, 6, 0, state)) {
                        return 0;
                    }
                    return 1;
                }
            } else if (endCol == 2 && state->whiteQueensideCastle) {
                if (isEmpty(board[7][1]) && isEmpty(board[7][2]) && isEmpty(board[7][3]) && 
                    board[7][0] == 'R') {
                    if (isSquareAttacked(board, 7, 4, 0, state) ||
                        isSquareAttacked(board, 7, 3, 0, state) ||
                        isSquareAttacked(board, 7, 2, 0, state)) {
                        return 0;
                    }
                    return 1;
                }
            }
        }
        // Black castling
        else if (!isWhite && startRow == 0 && startCol == 4) {
            if (endCol == 6 && state->blackKingsideCastle) {
                if (isEmpty(board[0][5]) && isEmpty(board[0][6]) && board[0][7] == 'r') {
                    if (isSquareAttacked(board, 0, 4, 1, state) ||
                        isSquareAttacked(board, 0, 5, 1, state) ||
                        isSquareAttacked(board, 0, 6, 1, state)) {
                        return 0;
                    }
                    return 1;
                }
            } else if (endCol == 2 && state->blackQueensideCastle) {
                if (isEmpty(board[0][1]) && isEmpty(board[0][2]) && isEmpty(board[0][3]) && 
                    board[0][0] == 'r') {
                    if (isSquareAttacked(board, 0, 4, 1, state) ||
                        isSquareAttacked(board, 0, 3, 1, state) ||
                        isSquareAttacked(board, 0, 2, 1, state)) {
                        return 0;
                    }
                    return 1;
                }
            }
        }
    }
    
    return 0;
}

// ============================================================================
// IMPROVED PAWN MOVE GENERATION WITH PROMOTION
// ============================================================================

static int generatePawnMoves(char board[8][8], int row, int col, Move moves[], int count, int whiteToMove, GameState* state) {
    char piece = board[row][col];
    int isWhite = isWhitePiece(piece);
    int direction = isWhite ? -1 : 1;
    int startRank = isWhite ? 6 : 1;
    int promotionRank = isWhite ? 0 : 7;
    
    // Single forward move
    int newRow = row + direction;
    if (newRow >= 0 && newRow < 8 && isEmpty(board[newRow][col])) {
        if (newRow == promotionRank) {
            // Generate all promotion options
            char promotionPieces[] = {'Q', 'R', 'B', 'N'};
            for (int i = 0; i < 4; i++) {
                if (isLegalMoveWithPromotion(board, row, col, newRow, col, whiteToMove, state, promotionPieces[i])) {
                    moves[count++] = (Move){row, col, newRow, col, promotionPieces[i]};
                }
            }
        } else {
            if (isLegalMove(board, row, col, newRow, col, whiteToMove, state)) {
                moves[count++] = (Move){row, col, newRow, col, 0};
            }
        }
        
        // Double forward move from starting position
        if (row == startRank) {
            int doubleRow = row + 2 * direction;
            if (doubleRow >= 0 && doubleRow < 8 && isEmpty(board[doubleRow][col]) && 
                isLegalMove(board, row, col, doubleRow, col, whiteToMove, state)) {
                moves[count++] = (Move){row, col, doubleRow, col, 0};
            }
        }
    }
    
    // Diagonal captures (including en passant)
    int captureCols[] = {col - 1, col + 1};
    for (int i = 0; i < 2; i++) {
        int newCol = captureCols[i];
        if (newCol < 0 || newCol >= 8) continue;
        
        int newRow = row + direction;
        if (newRow < 0 || newRow >= 8) continue;
        
        // Check if this is a legal capture (normal or en passant)
        if (isLegalMove(board, row, col, newRow, newCol, whiteToMove, state)) {
            if (newRow == promotionRank) {
                // Generate all promotion options for captures
                char promotionPieces[] = {'Q', 'R', 'B', 'N'};
                for (int j = 0; j < 4; j++) {
                    if (isLegalMoveWithPromotion(board, row, col, newRow, newCol, whiteToMove, state, promotionPieces[j])) {
                        moves[count++] = (Move){row, col, newRow, newCol, promotionPieces[j]};
                    }
                }
            } else {
                moves[count++] = (Move){row, col, newRow, newCol, 0};
            }
        }
    }
    return count;
}

// ============================================================================
// PIECE-SPECIFIC MOVE GENERATORS
// ============================================================================

static int generateKnightMoves(char board[8][8], int row, int col, Move moves[], int count, int whiteToMove, GameState* state) {
    for (int i = 0; i < 8; i++) {
        int newRow = row + knightOffsets[i][0];
        int newCol = col + knightOffsets[i][1];
        
        // Check bounds
        if (newRow < 0 || newRow >= 8 || newCol < 0 || newCol >= 8) continue;
        
        // Check if move is legal using your existing function
        if (isLegalMove(board, row, col, newRow, newCol, whiteToMove, state)) {
            moves[count].startRow = row;
            moves[count].startCol = col;
            moves[count].endRow = newRow;
            moves[count].endCol = newCol;
            moves[count].promotionPiece = 0;
            count++;
        }
    }
    return count;
}

static int generateBishopMoves(char board[8][8], int row, int col, Move moves[], int count, int whiteToMove, GameState* state) {
    for (int i = 0; i < 4; i++) {
        int rowStep = bishopOffsets[i][0];
        int colStep = bishopOffsets[i][1];
        int newRow = row + rowStep;
        int newCol = col + colStep;
        
        // Slide in this direction until we hit the board edge or a piece
        while (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8) {
            // Check if move is legal
            if (isLegalMove(board, row, col, newRow, newCol, whiteToMove, state)) {
                moves[count++] = (Move){row, col, newRow, newCol, 0};
            }
            
            // Stop if we hit a piece (we already included the capture in the check above)
            if (!isEmpty(board[newRow][newCol])) {
                break;
            }
            
            newRow += rowStep;
            newCol += colStep;
        }
    }
    return count;
}

static int generateRookMoves(char board[8][8], int row, int col, Move moves[], int count, int whiteToMove, GameState* state) {
    for (int i = 0; i < 4; i++) {
        int rowStep = rookOffsets[i][0];
        int colStep = rookOffsets[i][1];
        int newRow = row + rowStep;
        int newCol = col + colStep;
        
        // Slide in this direction until we hit the board edge or a piece
        while (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8) {
            // Check if move is legal
            if (isLegalMove(board, row, col, newRow, newCol, whiteToMove, state)) {
                moves[count++] = (Move){row, col, newRow, newCol, 0};
            }
            
            // Stop if we hit a piece
            if (!isEmpty(board[newRow][newCol])) {
                break;
            }
            
            newRow += rowStep;
            newCol += colStep;
        }
    }
    return count;
}

static int generateQueenMoves(char board[8][8], int row, int col, Move moves[], int count, int whiteToMove, GameState* state) {
    // Queen moves like bishop + rook
    count = generateBishopMoves(board, row, col, moves, count, whiteToMove, state);
    count = generateRookMoves(board, row, col, moves, count, whiteToMove, state);
    return count;
}

static int generateKingMoves(char board[8][8], int row, int col, Move moves[], int count, int whiteToMove, GameState* state) {
    // Normal king moves (one square in any direction)
    for (int i = 0; i < 8; i++) {
        int newRow = row + queenKingOffsets[i][0];
        int newCol = col + queenKingOffsets[i][1];
        
        if (newRow < 0 || newRow >= 8 || newCol < 0 || newCol >= 8) continue;
        
        if (isLegalMove(board, row, col, newRow, newCol, whiteToMove, state)) {
            moves[count++] = (Move){row, col, newRow, newCol, 0};
        }
    }
    
    // Add castling moves
    count = generateCastlingMoves(board, row, col, moves, count, whiteToMove, state);
    
    return count;
}

// ============================================================================
// MAIN MOVE VALIDATION (KEEP ORIGINAL)
// ============================================================================

int canPieceMoveTo(char board[8][8], int startRow, int startCol, int endRow, int endCol, 
                   GameState* state) {
    char piece = board[startRow][startCol];
    
    switch (toupper(piece)) {
        case 'P': return isValidPawnMove(board, startRow, startCol, endRow, endCol, state);
        case 'N': return isValidKnightMove(startRow, startCol, endRow, endCol);
        case 'B': return isValidBishopMove(board, startRow, startCol, endRow, endCol);
        case 'R': return isValidRookMove(board, startRow, startCol, endRow, endCol);
        case 'Q': return isValidQueenMove(board, startRow, startCol, endRow, endCol);
        case 'K': return isValidKingMove(board, startRow, startCol, endRow, endCol, state);
        default: return 0;
    }
}

int isLegalMove(char board[8][8], int startRow, int startCol, int endRow, int endCol, 
                int whiteToMove, GameState* state) {
    // Check basic validity
    if (!isCorrectColorMoving(board, startRow, startCol, whiteToMove)) return 0;
    if (!isNotCapturingSameColor(board, endRow, endCol, whiteToMove)) return 0;
    
    // Check piece-specific rules
    if (!canPieceMoveTo(board, startRow, startCol, endRow, endCol, state)) return 0;
    
    // Move cannot leave own king in check
    if (doesMovePutKingInCheck(board, startRow, startCol, endRow, endCol, whiteToMove, state)) {
        return 0;
    }
    
    return 1;
}

// ============================================================================
// IMPROVED MOVE GENERATION FUNCTION
// ============================================================================

int generateAllLegalMoves(char board[8][8], int whiteToMove, Move moves[], GameState* state) {
    int count = 0;
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            char piece = board[row][col];
            
            // Skip empty squares and pieces of wrong color
            if (isEmpty(piece)) continue;
            if (!isCorrectColorMoving(board, row, col, whiteToMove)) continue;
            
            // Generate moves based on piece type
            switch (toupper(piece)) {
                case 'P': 
                    count = generatePawnMoves(board, row, col, moves, count, whiteToMove, state);
                    break;
                case 'N':
                    count = generateKnightMoves(board, row, col, moves, count, whiteToMove, state);
                    break;
                case 'B':
                    count = generateBishopMoves(board, row, col, moves, count, whiteToMove, state);
                    break;
                case 'R':
                    count = generateRookMoves(board, row, col, moves, count, whiteToMove, state);
                    break;
                case 'Q':
                    count = generateQueenMoves(board, row, col, moves, count, whiteToMove, state);
                    break;
                case 'K':
                    count = generateKingMoves(board, row, col, moves, count, whiteToMove, state);
                    break;
            }
        }
    }
    return count;
}