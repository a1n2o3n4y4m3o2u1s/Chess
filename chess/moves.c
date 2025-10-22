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
// PIECE MOVEMENT RULES
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
// MAIN MOVE VALIDATION
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

int generateAllLegalMoves(char board[8][8], int whiteToMove, Move moves[], GameState* state) {
    int count = 0;
    
    for (int startRow = 0; startRow < 8; startRow++) {
        for (int startCol = 0; startCol < 8; startCol++) {
            if (isEmpty(board[startRow][startCol])) continue;
            
            for (int endRow = 0; endRow < 8; endRow++) {
                for (int endCol = 0; endCol < 8; endCol++) {
                    if (isLegalMove(board, startRow, startCol, endRow, endCol, whiteToMove, state)) {
                        moves[count].startRow = startRow;
                        moves[count].startCol = startCol;
                        moves[count].endRow = endRow;
                        moves[count].endCol = endCol;
                        count++;
                    }
                }
            }
        }
    }
    return count;
}