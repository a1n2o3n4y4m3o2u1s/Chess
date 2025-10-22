#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

// ============================================================================
// GAME STATE STRUCTURE
// ============================================================================
// Tracks castling rights and en passant opportunities
typedef struct {
    int whiteKingsideCastle;   // 1 if white can castle kingside, 0 otherwise
    int whiteQueensideCastle;  // 1 if white can castle queenside, 0 otherwise
    int blackKingsideCastle;   // 1 if black can castle kingside, 0 otherwise
    int blackQueensideCastle;  // 1 if black can castle queenside, 0 otherwise
    int enPassantCol;          // Column where en passant capture is possible (-1 if none)
    int enPassantRow;          // Row where en passant capture is possible
} GameState;

// ============================================================================
// MOVE STRUCTURE
// ============================================================================
// Represents a chess move with start and end positions
typedef struct {
    int startRow;
    int startCol;
    int endRow;
    int endCol;
} Move;

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================
void printBoard(char board[8][8]);
int rightColorMoving(char board[8][8], int startRow, int startCol, int whiteToMove);
int noSelfTake(char board[8][8], int endRow, int endCol, int whiteToMove);
int isLegalMove(char board[8][8], int startRow, int startCol, int endRow, int endCol, int whiteToMove, GameState* state);
int pawnLogic(char board[8][8], int startRow, int startCol, int endRow, int endCol, GameState* state);
int knightLogic(char board[8][8], int startRow, int startCol, int endRow, int endCol);
int bishopLogic(char board[8][8], int startRow, int startCol, int endRow, int endCol);
int rookLogic(char board[8][8], int startRow, int startCol, int endRow, int endCol);
int queenLogic(char board[8][8], int startRow, int startCol, int endRow, int endCol);
int kingLogic(char board[8][8], int startRow, int startCol, int endRow, int endCol, GameState* state);
int isSquareAttacked(char board[8][8], int row, int col, int byWhite, GameState* state);
int isInCheck(char board[8][8], int whiteKing, GameState* state);
int moveLeavesKingInCheck(char board[8][8], int startRow, int startCol, int endRow, int endCol, int whiteToMove, GameState* state);
int hasLegalMoves(char board[8][8], int whiteToMove, GameState* state);
int checkGameStatus(char board[8][8], int whiteToMove, GameState* state);

// ============================================================================
// GAME MODE CONSTANTS
// ============================================================================
#define MODE_PVP 0          // Player vs Player
#define MODE_PVB_WHITE 1    // Player vs Bot (player is white)
#define MODE_PVB_BLACK 2    // Player vs Bot (player is black)
#define MODE_BVB 3          // Bot vs Bot

// ============================================================================
// BOARD DISPLAY
// ============================================================================
// Prints the chess board to the console
// Board uses array indices [0-7][0-7] where [0][0] is a8 (top-left)
void printBoard(char board[8][8]) {
    printf("\n");
    for (int i = 0; i < 8; i++) {
        // Print rank numbers (8 to 1)
        printf("%d     ", 8 - i);
        
        // Print each square in the row
        for (int k = 0; k < 8; k++) {
            printf("%c   ", board[i][k]);
        }
        printf("\n\n");
    }
    // Print file letters (a to h)
    printf("\n      a   b   c   d   e   f   g   h\n\n");
}

// ============================================================================
// BASIC MOVE VALIDATION
// ============================================================================
// Checks if the piece at startRow, startCol belongs to the player whose turn it is
// Returns 1 if correct color is moving, 0 otherwise
int rightColorMoving(char board[8][8], int startRow, int startCol, int whiteToMove) {
    char piece = board[startRow][startCol];
    
    // White pieces are uppercase (PRNBQK), black pieces are lowercase (prnbqk)
    if (whiteToMove == 1) {
        return strchr("PRNBQK", piece) != NULL;
    } else {
        return strchr("prnbqk", piece) != NULL;
    }
}

// Checks if the move would capture the player's own piece
// Returns 1 if move is valid (not capturing own piece), 0 otherwise
int noSelfTake(char board[8][8], int endRow, int endCol, int whiteToMove) {
    char targetPiece = board[endRow][endCol];
    
    // Empty squares are always okay to move to
    if (targetPiece == '.') return 1;

    // Check that target piece is not the same color as moving piece
    if (whiteToMove == 1)
        return strchr("PRNBQK", targetPiece) == NULL;  // White can't capture white
    else
        return strchr("prnbqk", targetPiece) == NULL;  // Black can't capture black
}

// ============================================================================
// PIECE-SPECIFIC MOVEMENT LOGIC
// ============================================================================

// PAWN MOVEMENT RULES
// Pawns move forward one square, two squares from starting position,
// and capture diagonally. Also handles en passant.
int pawnLogic(char board[8][8], int startRow, int startCol, int endRow, int endCol, GameState* state) {
    char piece = board[startRow][startCol];
    int isWhite = isupper(piece);  // Uppercase = white, lowercase = black
    int direction = isWhite ? -1 : 1;  // White moves up (decreasing row), black moves down (increasing row)
    int rowDelta = endRow - startRow;
    int colDelta = abs(endCol - startCol);

    // FORWARD MOVE (no capture)
    if (colDelta == 0) {
        // Target square must be empty
        if (board[endRow][endCol] != '.') return 0;

        // Single square forward move
        if (rowDelta == direction) {
            return 1;
        }

        // Double square forward move (only from starting position)
        if (rowDelta == 2 * direction) {
            int startRank = isWhite ? 6 : 1;  // Row 6 for white (rank 2), row 1 for black (rank 7)
            if (startRow != startRank) return 0;  // Not on starting rank

            // Check that intermediate square is also empty
            int midRow = startRow + direction;
            if (board[midRow][startCol] != '.') return 0;

            return 1;
        }

        return 0;  // Invalid forward move
    }

    // DIAGONAL CAPTURE (normal capture or en passant)
    if (colDelta == 1 && rowDelta == direction) {
        // Normal capture - target square has opponent piece
        if (board[endRow][endCol] != '.') return 1;
        
        // En passant capture - capturing a pawn that just moved two squares
        if (state->enPassantCol == endCol && state->enPassantRow == endRow) {
            return 1;
        }
        
        return 0;  // Diagonal move to empty square (not en passant) is illegal
    }

    return 0;  // All other pawn moves are illegal
}

// KNIGHT MOVEMENT RULES
// Knights move in an L-shape: 2 squares in one direction, 1 square perpendicular
int knightLogic(char board[8][8], int startRow, int startCol, int endRow, int endCol) {
    int rowDist = abs(endRow - startRow);
    int colDist = abs(endCol - startCol);

    // Valid knight moves: (2,1) or (1,2) in any direction
    if ((rowDist == 2 && colDist == 1) || (rowDist == 1 && colDist == 2)) {
        return 1;  // Legal knight move
    }

    return 0;  // Illegal move
}

// BISHOP MOVEMENT RULES
// Bishops move diagonally any number of squares (path must be clear)
int bishopLogic(char board[8][8], int startRow, int startCol, int endRow, int endCol) {
    int rowDelta = abs(endRow - startRow);
    int colDelta = abs(endCol - startCol);

    // Must move equal distance in both row and column (diagonal)
    if (rowDelta != colDelta || rowDelta == 0) {
        return 0;
    }

    // Determine direction of movement (+1 or -1 for each axis)
    int rowStep = (endRow > startRow) ? 1 : -1;
    int colStep = (endCol > startCol) ? 1 : -1;

    // Check that all squares between start and end are empty
    int currentRow = startRow + rowStep;
    int currentCol = startCol + colStep;
    while (currentRow != endRow && currentCol != endCol) {
        if (board[currentRow][currentCol] != '.') {
            return 0;  // Path is blocked
        }
        currentRow += rowStep;
        currentCol += colStep;
    }

    return 1;  // Path is clear, diagonal move is valid
}

// ROOK MOVEMENT RULES
// Rooks move horizontally or vertically any number of squares (path must be clear)
int rookLogic(char board[8][8], int startRow, int startCol, int endRow, int endCol) {
    int rowDelta = abs(endRow - startRow);
    int colDelta = abs(endCol - startCol);

    // Must move in exactly one direction (horizontal OR vertical, not both)
    if (!((rowDelta == 0 && colDelta > 0) || (colDelta == 0 && rowDelta > 0))) {
        return 0;
    }

    // Determine direction of movement (0 if not moving in that axis)
    int rowStep = (rowDelta == 0) ? 0 : (endRow > startRow ? 1 : -1);
    int colStep = (colDelta == 0) ? 0 : (endCol > startCol ? 1 : -1);

    // Check that all squares between start and end are empty
    if (rowStep != 0) {
        // Moving vertically
        for (int r = startRow + rowStep; r != endRow; r += rowStep) {
            if (board[r][startCol] != '.') return 0;
        }
    } else {
        // Moving horizontally
        for (int c = startCol + colStep; c != endCol; c += colStep) {
            if (board[startRow][c] != '.') return 0;
        }
    }

    return 1;  // Path is clear, straight move is valid
}

// QUEEN MOVEMENT RULES
// Queens move like bishops OR rooks (diagonal, horizontal, or vertical)
int queenLogic(char board[8][8], int startRow, int startCol, int endRow, int endCol) {
    // Queen can move like a bishop OR like a rook
    return bishopLogic(board, startRow, startCol, endRow, endCol) ||
           rookLogic(board, startRow, startCol, endRow, endCol);
}

// KING MOVEMENT RULES
// Kings move one square in any direction, or castle (move two squares horizontally)
int kingLogic(char board[8][8], int startRow, int startCol, int endRow, int endCol, GameState* state) {
    int rowDelta = abs(endRow - startRow);
    int colDelta = abs(endCol - startCol);

    // NORMAL KING MOVE (one square in any direction)
    if (rowDelta <= 1 && colDelta <= 1) return 1;

    // CASTLING ATTEMPT (king moves 2 squares horizontally)
    if (rowDelta == 0 && colDelta == 2) {
        int isWhite = isupper(board[startRow][startCol]);
        
        // WHITE CASTLING
        if (isWhite && startRow == 7 && startCol == 4) {  // White king on e1
            if (endCol == 6 && state->whiteKingsideCastle) {  // Kingside castling (O-O)
                // Check that squares between king and rook are empty
                if (board[7][5] == '.' && board[7][6] == '.' && board[7][7] == 'R') {
                    // King cannot castle out of, through, or into check
                    if (isSquareAttacked(board, 7, 4, 0, state) ||  // Starting square
                        isSquareAttacked(board, 7, 5, 0, state) ||  // Passing through
                        isSquareAttacked(board, 7, 6, 0, state)) {  // Landing square
                        return 0;
                    }
                    return 1;
                }
            } else if (endCol == 2 && state->whiteQueensideCastle) {  // Queenside castling (O-O-O)
                // Check that squares between king and rook are empty
                if (board[7][1] == '.' && board[7][2] == '.' && board[7][3] == '.' && board[7][0] == 'R') {
                    // King cannot castle out of, through, or into check
                    if (isSquareAttacked(board, 7, 4, 0, state) ||  // Starting square
                        isSquareAttacked(board, 7, 3, 0, state) ||  // Passing through
                        isSquareAttacked(board, 7, 2, 0, state)) {  // Landing square
                        return 0;
                    }
                    return 1;
                }
            }
        }
        // BLACK CASTLING
        else if (!isWhite && startRow == 0 && startCol == 4) {  // Black king on e8
            if (endCol == 6 && state->blackKingsideCastle) {  // Kingside castling
                if (board[0][5] == '.' && board[0][6] == '.' && board[0][7] == 'r') {
                    if (isSquareAttacked(board, 0, 4, 1, state) ||
                        isSquareAttacked(board, 0, 5, 1, state) ||
                        isSquareAttacked(board, 0, 6, 1, state)) {
                        return 0;
                    }
                    return 1;
                }
            } else if (endCol == 2 && state->blackQueensideCastle) {  // Queenside castling
                if (board[0][1] == '.' && board[0][2] == '.' && board[0][3] == '.' && board[0][0] == 'r') {
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

    return 0;  // Invalid king move
}

// ============================================================================
// CHECK DETECTION
// ============================================================================

// Checks if a given square is under attack by a specific color
// Used to detect check and validate castling
// Returns 1 if square is attacked, 0 otherwise
int isSquareAttacked(char board[8][8], int row, int col, int byWhite, GameState* state) {
    // Scan all squares on the board
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            char piece = board[r][c];
            if (piece == '.') continue;  // Skip empty squares
            
            // Check if this piece belongs to the attacking color
            int isPieceWhite = isupper(piece);
            if (isPieceWhite != byWhite) continue;
            
            // Check if piece can legally attack the target square
            if (!rightColorMoving(board, r, c, byWhite)) continue;
            if (!noSelfTake(board, row, col, byWhite)) continue;
            
            // Test if this piece can move to the target square
            int canMove = 0;
            switch (toupper(piece)) {
                case 'P': canMove = pawnLogic(board, r, c, row, col, state); break;
                case 'N': canMove = knightLogic(board, r, c, row, col); break;
                case 'B': canMove = bishopLogic(board, r, c, row, col); break;
                case 'R': canMove = rookLogic(board, r, c, row, col); break;
                case 'Q': canMove = queenLogic(board, r, c, row, col); break;
                case 'K': {
                    // For king attacks, only check adjacent squares (no castling)
                    int rowDelta = abs(row - r);
                    int colDelta = abs(col - c);
                    canMove = (rowDelta <= 1 && colDelta <= 1);
                    break;
                }
            }
            
            if (canMove) return 1;  // Square is under attack
        }
    }
    return 0;  // Square is safe
}

// Checks if the king of a specific color is currently in check
// Returns 1 if king is in check, 0 otherwise
int isInCheck(char board[8][8], int whiteKing, GameState* state) {
    // Find the king on the board
    char kingChar = whiteKing ? 'K' : 'k';
    int kingRow = -1, kingCol = -1;
    
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            if (board[r][c] == kingChar) {
                kingRow = r;
                kingCol = c;
                break;
            }
        }
        if (kingRow != -1) break;
    }
    
    // If king not found (shouldn't happen in valid game), return 0
    if (kingRow == -1) return 0;
    
    // Check if opponent can attack the king's square
    return isSquareAttacked(board, kingRow, kingCol, !whiteKing, state);
}

// Checks if making a move would leave the player's own king in check
// This is done by temporarily making the move, checking for check, then undoing it
// Returns 1 if move leaves king in check (illegal), 0 otherwise
int moveLeavesKingInCheck(char board[8][8], int startRow, int startCol, 
                          int endRow, int endCol, int whiteToMove, GameState* state) {
    // Save the original board state
    char tempStart = board[startRow][startCol];
    char tempEnd = board[endRow][endCol];
    
    // Temporarily make the move
    board[endRow][endCol] = tempStart;
    board[startRow][startCol] = '.';
    
    // Check if the king is now in check
    int inCheck = isInCheck(board, whiteToMove, state);
    
    // Undo the move (restore original board state)
    board[startRow][startCol] = tempStart;
    board[endRow][endCol] = tempEnd;
    
    return inCheck;
}

// ============================================================================
// COMPLETE MOVE VALIDATION
// ============================================================================

// Checks if a move is legal according to all chess rules
// Returns 1 if legal, 0 if illegal
int isLegalMove(char board[8][8], int startRow, int startCol, int endRow, int endCol, int whiteToMove, GameState* state) {
    // Basic validation: right color moving and not capturing own piece
    if (!rightColorMoving(board, startRow, startCol, whiteToMove) || 
        !noSelfTake(board, endRow, endCol, whiteToMove))
        return 0;

    // Check piece-specific movement rules
    char piece = board[startRow][startCol];
    int legal = 0;
    
    switch (toupper(piece)) {
        case 'P': legal = pawnLogic(board, startRow, startCol, endRow, endCol, state); break;
        case 'N': legal = knightLogic(board, startRow, startCol, endRow, endCol); break;
        case 'B': legal = bishopLogic(board, startRow, startCol, endRow, endCol); break;
        case 'R': legal = rookLogic(board, startRow, startCol, endRow, endCol); break;
        case 'Q': legal = queenLogic(board, startRow, startCol, endRow, endCol); break;
        case 'K': legal = kingLogic(board, startRow, startCol, endRow, endCol, state); break;
    }
    
    if (!legal) return 0;
    
    // Final check: move cannot leave own king in check
    if (moveLeavesKingInCheck(board, startRow, startCol, endRow, endCol, whiteToMove, state)) {
        return 0;
    }
    
    return 1;  // Move is legal
}

// ============================================================================
// MOVE GENERATION (for bot)
// ============================================================================

// Generates all legal moves for the current player
// Stores moves in the provided array and returns the count
int generateLegalMoves(char board[8][8], int whiteToMove, Move moves[4096], GameState* state) {
    int numMoves = 0;
    
    // Try every possible start square
    for (int startRow = 0; startRow < 8; startRow++) {
        for (int startCol = 0; startCol < 8; startCol++) {
            if (board[startRow][startCol] == '.') continue;  // Skip empty squares

            // Try every possible destination square
            for (int endRow = 0; endRow < 8; endRow++) {
                for (int endCol = 0; endCol < 8; endCol++) {
                    // Check if this move is legal
                    if (isLegalMove(board, startRow, startCol, endRow, endCol, whiteToMove, state)) {
                        moves[numMoves].startRow = startRow;
                        moves[numMoves].startCol = startCol;
                        moves[numMoves].endRow = endRow;
                        moves[numMoves].endCol = endCol;
                        numMoves++;
                    }
                }
            }
        }
    }
    return numMoves;
}

// Checks if the current player has any legal moves
// Returns 1 if there are legal moves, 0 if none (checkmate or stalemate)
int hasLegalMoves(char board[8][8], int whiteToMove, GameState* state) {
    Move moves[4096];
    int numMoves = generateLegalMoves(board, whiteToMove, moves, state);
    return numMoves > 0;
}

// ============================================================================
// GAME STATUS CHECKING
// ============================================================================

// Checks for checkmate, stalemate, or check
// Returns 1 if game is over (checkmate or stalemate), 0 if game continues
int checkGameStatus(char board[8][8], int whiteToMove, GameState* state) {
    int inCheck = isInCheck(board, whiteToMove, state);
    int hasLegal = hasLegalMoves(board, whiteToMove, state);
    
    // No legal moves available
    if (!hasLegal) {
        if (inCheck) {
            // In check with no legal moves = checkmate
            printf("\n*** CHECKMATE! %s wins! ***\n\n", whiteToMove ? "Black" : "White");
        } else {
            // Not in check but no legal moves = stalemate
            printf("\n*** STALEMATE! Draw. ***\n\n");
        }
        return 1;  // Game over
    }
    
    // King is in check but there are legal moves (can escape)
    if (inCheck) {
        printf(">>> Check! <<<\n");
    }
    
    return 0;  // Game continues
}

// ============================================================================
// BOT AI
// ============================================================================

// Evaluates the current board position from white's perspective
// Positive score = white is winning, negative score = black is winning
// This is a simple material-based evaluation (can be enhanced later)
int evaluateBoard(char board[8][8]) {
    int score = 0;
    
    // Piece values (standard chess values in centipawns)
    // Pawn = 100, Knight = 320, Bishop = 330, Rook = 500, Queen = 900, King = 20000
    
    // Scan entire board and sum piece values
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            char piece = board[row][col];
            
            // Skip empty squares
            if (piece == '.') continue;
            
            // Determine piece value
            int pieceValue = 0;
            switch (toupper(piece)) {
                case 'P': pieceValue = 100; break;   // Pawn
                case 'N': pieceValue = 320; break;   // Knight
                case 'B': pieceValue = 330; break;   // Bishop
                case 'R': pieceValue = 500; break;   // Rook
                case 'Q': pieceValue = 900; break;   // Queen
                case 'K': pieceValue = 20000; break; // King (very high value, losing king = game over)
            }
            
            // Add to score if white piece, subtract if black piece
            if (isupper(piece)) {
                score += pieceValue;  // White piece (uppercase)
            } else {
                score -= pieceValue;  // Black piece (lowercase)
            }
        }
    }
    
    return score;
}

// Add this function before getBotMove() - around line 530

// ============================================================================
// MINIMAX SEARCH WITH ALPHA-BETA PRUNING
// ============================================================================

// Minimax algorithm: recursively searches game tree to find best move
// Alpha-Beta pruning eliminates branches that can't affect final decision
// Returns best evaluation score for current player
int minimax(char board[8][8], GameState* state, int depth, int alpha, int beta, int maximizing) {
    // Base case: reached maximum depth or game over
    if (depth == 0 || !hasLegalMoves(board, maximizing, state)) {
        return evaluateBoard(board);
    }
    
    // Generate all legal moves for current player
    Move moves[4096];
    int numMoves = generateLegalMoves(board, maximizing, moves, state);
    
    if (maximizing) {
        // Maximizing player (white) - find highest score
        int maxEval = -999999;
        
        for (int i = 0; i < numMoves; i++) {
            // Save board state
            char tempStart = board[moves[i].startRow][moves[i].startCol];
            char tempEnd = board[moves[i].endRow][moves[i].endCol];
            char capturedPawn = '.';
            
            // Handle en passant capture
            int isEnPassant = 0;
            if (toupper(tempStart) == 'P' && moves[i].endCol != moves[i].startCol && tempEnd == '.') {
                capturedPawn = board[moves[i].startRow][moves[i].endCol];
                board[moves[i].startRow][moves[i].endCol] = '.';
                isEnPassant = 1;
            }
            
            // Make move
            board[moves[i].endRow][moves[i].endCol] = tempStart;
            board[moves[i].startRow][moves[i].startCol] = '.';
            
            // Save game state
            GameState tempState = *state;
            
            // Update en passant
            state->enPassantCol = -1;
            if (toupper(tempStart) == 'P' && abs(moves[i].endRow - moves[i].startRow) == 2) {
                state->enPassantCol = moves[i].endCol;
                state->enPassantRow = (moves[i].startRow + moves[i].endRow) / 2;
            }
            
            // Recursive call for opponent's response
            int eval = minimax(board, state, depth - 1, alpha, beta, 0);
            
            // Undo move
            board[moves[i].startRow][moves[i].startCol] = tempStart;
            board[moves[i].endRow][moves[i].endCol] = tempEnd;
            if (isEnPassant) {
                board[moves[i].startRow][moves[i].endCol] = capturedPawn;
            }
            *state = tempState;
            
            // Update best score and alpha
            maxEval = (eval > maxEval) ? eval : maxEval;
            alpha = (eval > alpha) ? eval : alpha;

            
            // Beta cutoff - opponent won't allow this branch
            if (beta <= alpha) break;
        }
        return maxEval;
        
    } else {
        // Minimizing player (black) - find lowest score
        int minEval = 999999;
        
        for (int i = 0; i < numMoves; i++) {
            // Save board state
            char tempStart = board[moves[i].startRow][moves[i].startCol];
            char tempEnd = board[moves[i].endRow][moves[i].endCol];
            char capturedPawn = '.';
            
            // Handle en passant
            int isEnPassant = 0;
            if (toupper(tempStart) == 'P' && moves[i].endCol != moves[i].startCol && tempEnd == '.') {
                capturedPawn = board[moves[i].startRow][moves[i].endCol];
                board[moves[i].startRow][moves[i].endCol] = '.';
                isEnPassant = 1;
            }
            
            // Make move
            board[moves[i].endRow][moves[i].endCol] = tempStart;
            board[moves[i].startRow][moves[i].startCol] = '.';
            
            // Save game state
            GameState tempState = *state;
            
            // Update en passant
            state->enPassantCol = -1;
            if (toupper(tempStart) == 'P' && abs(moves[i].endRow - moves[i].startRow) == 2) {
                state->enPassantCol = moves[i].endCol;
                state->enPassantRow = (moves[i].startRow + moves[i].endRow) / 2;
            }
            
            // Recursive call for opponent's response
            int eval = minimax(board, state, depth - 1, alpha, beta, 1);
            
            // Undo move
            board[moves[i].startRow][moves[i].startCol] = tempStart;
            board[moves[i].endRow][moves[i].endCol] = tempEnd;
            if (isEnPassant) {
                board[moves[i].startRow][moves[i].endCol] = capturedPawn;
            }
            *state = tempState;
            
            // Update best score and beta
            minEval = (eval < minEval) ? eval : minEval;
            beta = (eval < beta) ? eval : beta;
            
            // Alpha cutoff
            if (beta <= alpha) break;
        }
        return minEval;
    }
}

// REPLACE the getBotMove() function with this improved version:
// Selects a move for the bot
// Current implementation: evaluation-based move selection
// Evaluates each possible move and picks the best one
void getBotMove(char board[8][8], int whiteToMove, int* startRow, int* startCol, int* endRow, int* endCol, GameState* state) {
    Move moves[4096];
    int numMoves = generateLegalMoves(board, whiteToMove, moves, state);
    
    if (numMoves == 0) {
        *startRow = -1;
        *startCol = -1;
        *endRow = -1;
        *endCol = -1;
        return;
    }
    
    int bestMoveIndex = 0;
    int bestScore = whiteToMove ? -999999 : 999999;
    
    // ADJUST THIS VALUE to change thinking depth
    // 2 = looks 1 move ahead (your move + opponent response)
    // 4 = looks 2 moves ahead (your move + their response + your response + their response)
    // 6 = looks 3 moves ahead, etc.
    int searchDepth = 6;  // Recommended: 4-6 for good play
    
    printf("Searching %d moves deep...\n", searchDepth / 2);
    
    for (int i = 0; i < numMoves; i++) {
        // Save board state
        char tempStart = board[moves[i].startRow][moves[i].startCol];
        char tempEnd = board[moves[i].endRow][moves[i].endCol];
        char capturedPawn = '.';
        
        // Handle en passant
        int isEnPassant = 0;
        if (toupper(tempStart) == 'P' && moves[i].endCol != moves[i].startCol && tempEnd == '.') {
            capturedPawn = board[moves[i].startRow][moves[i].endCol];
            board[moves[i].startRow][moves[i].endCol] = '.';
            isEnPassant = 1;
        }
        
        // Make move
        board[moves[i].endRow][moves[i].endCol] = tempStart;
        board[moves[i].startRow][moves[i].startCol] = '.';
        
        // Save game state
        GameState tempState = *state;
        
        // Update en passant
        state->enPassantCol = -1;
        if (toupper(tempStart) == 'P' && abs(moves[i].endRow - moves[i].startRow) == 2) {
            state->enPassantCol = moves[i].endCol;
            state->enPassantRow = (moves[i].startRow + moves[i].endRow) / 2;
        }
        
        // Search opponent's responses using minimax
        int score = minimax(board, state, searchDepth - 1, -999999, 999999, !whiteToMove);
        
        // Undo move
        board[moves[i].startRow][moves[i].startCol] = tempStart;
        board[moves[i].endRow][moves[i].endCol] = tempEnd;
        if (isEnPassant) {
            board[moves[i].startRow][moves[i].endCol] = capturedPawn;
        }
        *state = tempState;
        
        // Check if this is the best move
        if (whiteToMove) {
            if (score > bestScore) {
                bestScore = score;
                bestMoveIndex = i;
            }
        } else {
            if (score < bestScore) {
                bestScore = score;
                bestMoveIndex = i;
            }
        }
    }
    
    printf("Best move score: %d\n", bestScore);
    
    *startRow = moves[bestMoveIndex].startRow;
    *startCol = moves[bestMoveIndex].startCol;
    *endRow = moves[bestMoveIndex].endRow;
    *endCol = moves[bestMoveIndex].endCol;
}

// ============================================================================
// MAIN GAME LOOP
// ============================================================================

int main() {
    // Disable output buffering for immediate console display
    setvbuf(stdout, NULL, _IONBF, 0);
    
    // Seed random number generator for bot moves
    srand(time(NULL));

    // Initialize chess board (8x8 array)
    // Uppercase = white pieces, lowercase = black pieces, '.' = empty square
    // Board layout: [0][0] is a8 (top-left), [7][7] is h1 (bottom-right)
    char board[8][8] = {
        {'r','n','b','q','k','b','n','r'},  // Rank 8 (black back rank)
        {'p','p','p','p','p','p','p','p'},  // Rank 7 (black pawns)
        {'.','.','.','.','.','.','.','.'},  // Rank 6 (empty)
        {'.','.','.','.','.','.','.','.'},  // Rank 5 (empty)
        {'.','.','.','.','.','.','.','.'},  // Rank 4 (empty)
        {'.','.','.','.','.','.','.','.'},  // Rank 3 (empty)
        {'P','P','P','P','P','P','P','P'},  // Rank 2 (white pawns)
        {'R','N','B','Q','K','B','N','R'}   // Rank 1 (white back rank)
    };

    // Initialize game state (castling rights and en passant)
    GameState state;
    state.whiteKingsideCastle = 1;   // White can castle kingside initially
    state.whiteQueensideCastle = 1;  // White can castle queenside initially
    state.blackKingsideCastle = 1;   // Black can castle kingside initially
    state.blackQueensideCastle = 1;  // Black can castle queenside initially
    state.enPassantCol = -1;         // No en passant opportunity at start
    state.enPassantRow = -1;

    char input[10];           // Buffer for user input
    int whiteToMove = 1;      // 1 = white's turn, 0 = black's turn
    int gameMode = -1;        // Game mode (PVP, PVB, BVB) - set by user

    // Display welcome message and instructions
    printf("Welcome to Chess Terminal. Commands:\n");
    printf("- 'pvp' : Player vs Player\n");
    printf("- 'pvb' : Player vs Bot\n");
    printf("- 'bvb' : Bot vs Bot\n");
    printf("- 'next' : Advance to next bot move (when bot is playing)\n");
    printf("- 'quit' : Exit\n");
    printf("- Move format: e2e4\n\n");

    // Initial mode selection loop
    printf("Select game mode: pvp, pvb, bvb\n");
    while (gameMode == -1) {
        scanf("%9s", input);
        
        if (strcmp(input, "pvp") == 0) {
            // Player vs Player mode
            gameMode = MODE_PVP;
            printf("Mode set to Player vs Player.\n");
        } else if (strcmp(input, "pvb") == 0) {
            // Player vs Bot mode - ask which color player wants
            printf("Play as white or black? (enter 'white' or 'black')\n");
            char colorChoice[10];
            scanf("%9s", colorChoice);
            if (strcmp(colorChoice, "white") == 0) {
                gameMode = MODE_PVB_WHITE;
                printf("Mode set to Player vs Bot. You play white.\n");
            } else if (strcmp(colorChoice, "black") == 0) {
                gameMode = MODE_PVB_BLACK;
                printf("Mode set to Player vs Bot. You play black.\n");
            } else {
                printf("Invalid color choice. Please select pvb again.\n");
            }
        } else if (strcmp(input, "bvb") == 0) {
            // Bot vs Bot mode - user advances moves manually
            gameMode = MODE_BVB;
            printf("Mode set to Bot vs Bot. Type 'next' after each move to continue.\n");
        } else {
            printf("Invalid mode. Select pvp, pvb, or bvb.\n");
        }
    }

    // ========================================================================
    // MAIN GAME LOOP
    // ========================================================================
    while (1) {
        // Display the current board state
        printBoard(board);

        // Check for checkmate, stalemate, or check
        if (checkGameStatus(board, whiteToMove, &state)) {
            break;  // Game is over
        }

        // Determine if it's the bot's turn based on game mode
        int isBotTurn = 0;
        if (gameMode == MODE_BVB) {
            // Bot vs Bot: always bot turn
            isBotTurn = 1;
        } else if (gameMode == MODE_PVB_WHITE && whiteToMove == 0) {
            // Player is white, black to move = bot's turn
            isBotTurn = 1;
        } else if (gameMode == MODE_PVB_BLACK && whiteToMove == 1) {
            // Player is black, white to move = bot's turn
            isBotTurn = 1;
        }

        int startRow, startCol, endRow, endCol;  // Move coordinates
        
        if (isBotTurn) {
            // ================================================================
            // BOT'S TURN
            // ================================================================
            // Wait for user to type 'next' before bot makes its move
            printf("Bot ready to move. Type 'next' to continue (or 'quit' to exit):\n");
            scanf("%9s", input);
            
            if (strcmp(input, "quit") == 0) {
                break;  // Exit game loop
            } else if (strcmp(input, "next") != 0) {
                printf("Invalid command. Use 'next' to proceed or 'quit' to exit.\n");
                continue;  // Ask for input again
            }
            
            // Bot calculates and makes a move
            printf("Bot thinking...\n");
            getBotMove(board, whiteToMove, &startRow, &startCol, &endRow, &endCol, &state);
            
            // Check if bot found a legal move
            if (startRow == -1) {
                printf("No legal moves for bot. Game over?\n");
                break;
            }
            
            // Display bot's move in algebraic notation
            printf("Bot moves %c%d%c%d\n", 'a' + startCol, 8 - startRow, 'a' + endCol, 8 - endRow);
            
        } else {
            // ================================================================
            // HUMAN PLAYER'S TURN
            // ================================================================
            // Prompt for move based on whose turn it is
            if (whiteToMove == 1) {
                printf("Enter move or command (White to move):\n");
            } else {
                printf("Enter move or command (Black to move):\n");
            }
            scanf("%9s", input);

            // Check for game mode change commands
            if (strcmp(input, "pvp") == 0) {
                gameMode = MODE_PVP;
                printf("Mode set to Player vs Player.\n");
                continue;  // Don't process as a move, go to next iteration
            } else if (strcmp(input, "pvb") == 0) {
                printf("Play as white or black? (enter 'white' or 'black')\n");
                char colorChoice[10];
                scanf("%9s", colorChoice);
                if (strcmp(colorChoice, "white") == 0) {
                    gameMode = MODE_PVB_WHITE;
                    printf("Mode set to Player vs Bot. You play white.\n");
                } else if (strcmp(colorChoice, "black") == 0) {
                    gameMode = MODE_PVB_BLACK;
                    printf("Mode set to Player vs Bot. You play black.\n");
                } else {
                    printf("Invalid color choice.\n");
                }
                continue;
            } else if (strcmp(input, "bvb") == 0) {
                gameMode = MODE_BVB;
                printf("Mode set to Bot vs Bot. Type 'next' after each move to continue.\n");
                continue;
            } else if (strcmp(input, "quit") == 0) {
                break;  // Exit game loop
            }

            // Parse move input (expected format: e2e4)
            if (strlen(input) != 4) {
                printf("Invalid input. Try again.\n");
                continue;
            }

            // Convert algebraic notation to array indices
            // Files: 'a'=0, 'b'=1, ..., 'h'=7
            // Ranks: '8'=0, '7'=1, ..., '1'=7
            startCol = input[0] - 'a';        // Starting file
            startRow = 8 - (input[1] - '0');  // Starting rank
            endCol   = input[2] - 'a';        // Ending file
            endRow   = 8 - (input[3] - '0');  // Ending rank
        }

        // Validate move coordinates are within board boundaries
        if (startRow < 0 || startRow > 7 || startCol < 0 || startCol > 7 ||
            endRow < 0 || endRow > 7 || endCol < 0 || endCol > 7) {
            printf("Invalid move coordinates.\n");
            if (!isBotTurn) continue;  // Let human try again
            else break;  // Bot error (shouldn't happen)
        }

        // ====================================================================
        // EXECUTE MOVE
        // ====================================================================
        // Check if the move is legal according to all chess rules
        if (isLegalMove(board, startRow, startCol, endRow, endCol, whiteToMove, &state) == 1) {
            // Display move confirmation
            printf("Move executed: %c from %c%d to %c%d\n", 
                   board[startRow][startCol], 
                   'a' + startCol, 8 - startRow, 
                   'a' + endCol, 8 - endRow);
            
            // ----------------------------------------------------------------
            // HANDLE EN PASSANT CAPTURE
            // ----------------------------------------------------------------
            // En passant: pawn moves diagonally to empty square (captured pawn is beside it)
            if (toupper(board[startRow][startCol]) == 'P' && 
                endCol != startCol && 
                board[endRow][endCol] == '.') {
                // Remove the captured pawn (it's on the same row as starting position)
                board[startRow][endCol] = '.';
                printf("En passant capture!\n");
            }
            
            // ----------------------------------------------------------------
            // MOVE THE PIECE
            // ----------------------------------------------------------------
            board[endRow][endCol] = board[startRow][startCol];
            board[startRow][startCol] = '.';
            
            // ----------------------------------------------------------------
            // HANDLE CASTLING
            // ----------------------------------------------------------------
            // When king moves 2 squares horizontally, move the rook too
            if (toupper(board[endRow][endCol]) == 'K' && abs(endCol - startCol) == 2) {
                if (endCol == 6) {  // Kingside castling (O-O)
                    // Move rook from h-file to f-file
                    board[endRow][5] = board[endRow][7];
                    board[endRow][7] = '.';
                    printf("Castled kingside!\n");
                } else if (endCol == 2) {  // Queenside castling (O-O-O)
                    // Move rook from a-file to d-file
                    board[endRow][3] = board[endRow][0];
                    board[endRow][0] = '.';
                    printf("Castled queenside!\n");
                }
            }
            
            // ----------------------------------------------------------------
            // HANDLE PAWN PROMOTION
            // ----------------------------------------------------------------
            // Pawn reaches opposite end of board (row 0 for white, row 7 for black)
            if (toupper(board[endRow][endCol]) == 'P') {
                if (endRow == 0 || endRow == 7) {
                    if (isBotTurn) {
                        // Bot always promotes to queen (strongest piece)
                        board[endRow][endCol] = (endRow == 0) ? 'Q' : 'q';
                        printf("Pawn promoted to Queen!\n");
                    } else {
                        // Human player chooses promotion piece
                        printf("Pawn promotion! Choose piece (Q/R/B/N): ");
                        char promoChoice;
                        scanf(" %c", &promoChoice);
                        promoChoice = toupper(promoChoice);
                        // Default to queen if invalid choice
                        if (strchr("QRBN", promoChoice) == NULL) promoChoice = 'Q';
                        // Set correct case based on which player promoted
                        board[endRow][endCol] = (endRow == 0) ? promoChoice : tolower(promoChoice);
                        printf("Pawn promoted to %c!\n", promoChoice);
                    }
                }
            }
            
            // ----------------------------------------------------------------
            // UPDATE CASTLING RIGHTS
            // ----------------------------------------------------------------
            // Castling rights are lost when king or rook moves
            
            // If king moved, lose both castling rights for that color
            if (toupper(board[endRow][endCol]) == 'K') {
                if (whiteToMove) {
                    state.whiteKingsideCastle = 0;
                    state.whiteQueensideCastle = 0;
                } else {
                    state.blackKingsideCastle = 0;
                    state.blackQueensideCastle = 0;
                }
            }
            
            // If rook moved from starting position, lose that side's castling right
            if (startRow == 7 && startCol == 0) state.whiteQueensideCastle = 0;  // White queenside rook
            if (startRow == 7 && startCol == 7) state.whiteKingsideCastle = 0;   // White kingside rook
            if (startRow == 0 && startCol == 0) state.blackQueensideCastle = 0;  // Black queenside rook
            if (startRow == 0 && startCol == 7) state.blackKingsideCastle = 0;   // Black kingside rook
            
            // If rook was captured on starting square, lose that side's castling right
            if (endRow == 7 && endCol == 0) state.whiteQueensideCastle = 0;
            if (endRow == 7 && endCol == 7) state.whiteKingsideCastle = 0;
            if (endRow == 0 && endCol == 0) state.blackQueensideCastle = 0;
            if (endRow == 0 && endCol == 7) state.blackKingsideCastle = 0;
            
            // ----------------------------------------------------------------
            // SET EN PASSANT OPPORTUNITY
            // ----------------------------------------------------------------
            // Reset en passant opportunity (it's only valid for one turn)
            state.enPassantCol = -1;
            
            // If pawn moved two squares, set en passant opportunity for opponent
            if (toupper(board[endRow][endCol]) == 'P' && abs(endRow - startRow) == 2) {
                state.enPassantCol = endCol;  // Column where en passant capture is possible
                state.enPassantRow = (startRow + endRow) / 2;  // Row of the square "behind" the pawn
            }

            // ----------------------------------------------------------------
            // SWITCH TURNS
            // ----------------------------------------------------------------
            whiteToMove = !whiteToMove;  // Toggle between white (1) and black (0)
            
        } else {
            // ================================================================
            // ILLEGAL MOVE HANDLING
            // ================================================================
            printf("Illegal move! Try again.\n");
            if (isBotTurn) {
                // Bot should never generate illegal moves (this indicates a bug)
                printf("Bot error - invalid move generated.\n");
                break;
            }
            // Human can try again - loop continues
        }
    }

    // Game has ended
    printf("Game ended.\n");
    return 0;
}