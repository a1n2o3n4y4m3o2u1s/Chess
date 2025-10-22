#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <board.h>
#include <moves.h>
#include <gameState.h>
#include <bot.h>

// ============================================================================
// GAME MODES
// ============================================================================
#define MODE_PVP 0          // Player vs Player
#define MODE_PVB_WHITE 1    // Player vs Bot (player is white)
#define MODE_PVB_BLACK 2    // Player vs Bot (player is black)
#define MODE_BVB 3          // Bot vs Bot

// ============================================================================
// GAME LOGIC HELPERS
// ============================================================================

static void updateCastlingRights(GameState* state, char board[8][8], 
                                 int startRow, int startCol, int endRow, int endCol) {
    char piece = board[endRow][endCol];
    
    // King moved - lose all castling rights for that color
    if (toupper(piece) == 'K') {
        if (isWhitePiece(piece)) {
            state->whiteKingsideCastle = 0;
            state->whiteQueensideCastle = 0;
        } else {
            state->blackKingsideCastle = 0;
            state->blackQueensideCastle = 0;
        }
    }
    
    // Rook moved from starting position
    if (startRow == 7 && startCol == 0) state->whiteQueensideCastle = 0;
    if (startRow == 7 && startCol == 7) state->whiteKingsideCastle = 0;
    if (startRow == 0 && startCol == 0) state->blackQueensideCastle = 0;
    if (startRow == 0 && startCol == 7) state->blackKingsideCastle = 0;
    
    // Rook captured on starting position
    if (endRow == 7 && endCol == 0) state->whiteQueensideCastle = 0;
    if (endRow == 7 && endCol == 7) state->whiteKingsideCastle = 0;
    if (endRow == 0 && endCol == 0) state->blackQueensideCastle = 0;
    if (endRow == 0 && endCol == 7) state->blackKingsideCastle = 0;
}

static void handleEnPassantCapture(char board[8][8], int startRow, int startCol, 
                                   int endRow, int endCol) {
    char piece = board[endRow][endCol];
    
    // Check if this is an en passant capture
    if (toupper(piece) == 'P' && endCol != startCol && isEmpty(board[endRow][endCol])) {
        board[startRow][endCol] = '.';  // Remove captured pawn
        printf("En passant capture!\n");
    }
}

static void handleCastling(char board[8][8], int startCol, int endRow, int endCol) {
    char piece = board[endRow][endCol];
    
    if (toupper(piece) == 'K' && abs(endCol - startCol) == 2) {
        if (endCol == 6) {  // Kingside
            board[endRow][5] = board[endRow][7];
            board[endRow][7] = '.';
            printf("Castled kingside!\n");
        } else if (endCol == 2) {  // Queenside
            board[endRow][3] = board[endRow][0];
            board[endRow][0] = '.';
            printf("Castled queenside!\n");
        }
    }
}

static void handlePawnPromotion(char board[8][8], int endRow, int endCol, int isBotMove) {
    char piece = board[endRow][endCol];
    
    if (toupper(piece) != 'P') return;
    if (endRow != 0 && endRow != 7) return;
    
    if (isBotMove) {
        // Bot always promotes to queen
        board[endRow][endCol] = (endRow == 0) ? 'Q' : 'q';
        printf("Pawn promoted to Queen!\n");
    } else {
        // Player chooses promotion piece
        printf("Pawn promotion! Choose piece (Q/R/B/N): ");
        char choice;
        scanf(" %c", &choice);
        choice = toupper(choice);
        
        if (strchr("QRBN", choice) == NULL) choice = 'Q';  // Default to queen
        
        board[endRow][endCol] = (endRow == 0) ? choice : tolower(choice);
        printf("Pawn promoted to %c!\n", choice);
    }
}

static void updateEnPassantState(GameState* state, char board[8][8], 
                                int startRow, int endRow, int endCol) {
    char piece = board[endRow][endCol];
    state->enPassantCol = -1;
    
    // Pawn moved two squares - set en passant opportunity
    if (toupper(piece) == 'P' && abs(endRow - startRow) == 2) {
        state->enPassantCol = endCol;
        state->enPassantRow = (startRow + endRow) / 2;
    }
}

static void executeMove(char board[8][8], GameState* state, int startRow, int startCol, 
                       int endRow, int endCol, int isBotMove) {
    printf("Move executed: %c from %c%d to %c%d\n", 
           board[startRow][startCol], 
           'a' + startCol, 8 - startRow, 
           'a' + endCol, 8 - endRow);
    
    handleEnPassantCapture(board, startRow, startCol, endRow, endCol);
    
    // Move the piece
    board[endRow][endCol] = board[startRow][startCol];
    board[startRow][startCol] = '.';
    
    handleCastling(board, startCol, endRow, endCol);
    handlePawnPromotion(board, endRow, endCol, isBotMove);
    
    updateCastlingRights(state, board, startRow, startCol, endRow, endCol);
    updateEnPassantState(state, board, startRow, endRow, endCol);
}

// ============================================================================
// USER INPUT HANDLING
// ============================================================================

static int parseMove(char* input, int* startRow, int* startCol, int* endRow, int* endCol) {
    if (strlen(input) != 4) return 0;
    
    *startCol = input[0] - 'a';
    *startRow = 8 - (input[1] - '0');
    *endCol = input[2] - 'a';
    *endRow = 8 - (input[3] - '0');
    
    // Validate coordinates are in bounds
    if (*startRow < 0 || *startRow > 7 || *startCol < 0 || *startCol > 7) return 0;
    if (*endRow < 0 || *endRow > 7 || *endCol < 0 || *endCol > 7) return 0;
    
    return 1;
}

static int isBotTurn(int gameMode, int whiteToMove) {
    if (gameMode == MODE_BVB) return 1;
    if (gameMode == MODE_PVB_WHITE && !whiteToMove) return 1;
    if (gameMode == MODE_PVB_BLACK && whiteToMove) return 1;
    return 0;
}

// ============================================================================
// MODE SELECTION
// ============================================================================

static int selectGameMode() {
    printf("Welcome to Chess Terminal. Commands:\n");
    printf("- 'pvp' : Player vs Player\n");
    printf("- 'pvb' : Player vs Bot\n");
    printf("- 'bvb' : Bot vs Bot\n");
    printf("- 'next' : Advance to next bot move (when bot is playing)\n");
    printf("- 'quit' : Exit\n");
    printf("- Move format: e2e4\n\n");
    
    printf("Select game mode: pvp, pvb, bvb\n");
    
    char input[10];
    while (1) {
        scanf("%9s", input);
        
        if (strcmp(input, "pvp") == 0) {
            printf("Mode set to Player vs Player.\n");
            return MODE_PVP;
        } 
        else if (strcmp(input, "pvb") == 0) {
            printf("Play as white or black? (enter 'white' or 'black')\n");
            char color[10];
            scanf("%9s", color);
            
            if (strcmp(color, "white") == 0) {
                printf("Mode set to Player vs Bot. You play white.\n");
                return MODE_PVB_WHITE;
            } else if (strcmp(color, "black") == 0) {
                printf("Mode set to Player vs Bot. You play black.\n");
                return MODE_PVB_BLACK;
            } else {
                printf("Invalid color choice. Please select pvb again.\n");
            }
        } 
        else if (strcmp(input, "bvb") == 0) {
            printf("Mode set to Bot vs Bot. Type 'next' after each move to continue.\n");
            return MODE_BVB;
        } 
        else {
            printf("Invalid mode. Select pvp, pvb, or bvb.\n");
        }
    }
}

static int handleModeChange(char* input, int* gameMode) {
    if (strcmp(input, "pvp") == 0) {
        *gameMode = MODE_PVP;
        printf("Mode set to Player vs Player.\n");
        return 1;
    } 
    else if (strcmp(input, "pvb") == 0) {
        printf("Play as white or black? (enter 'white' or 'black')\n");
        char color[10];
        scanf("%9s", color);
        
        if (strcmp(color, "white") == 0) {
            *gameMode = MODE_PVB_WHITE;
            printf("Mode set to Player vs Bot. You play white.\n");
        } else if (strcmp(color, "black") == 0) {
            *gameMode = MODE_PVB_BLACK;
            printf("Mode set to Player vs Bot. You play black.\n");
        } else {
            printf("Invalid color choice.\n");
        }
        return 1;
    } 
    else if (strcmp(input, "bvb") == 0) {
        *gameMode = MODE_BVB;
        printf("Mode set to Bot vs Bot. Type 'next' after each move to continue.\n");
        return 1;
    }
    return 0;
}

// ============================================================================
// MAIN GAME LOOP
// ============================================================================

int main() {
    setvbuf(stdout, NULL, _IONBF, 0);
    srand(time(NULL));
    
    char board[8][8];
    GameState state;
    
    initializeBoard(board);
    initializeGameState(&state);
    
    int whiteToMove = 1;
    int gameMode = selectGameMode();
    
    while (1) {
        printBoard(board);
        
        if (checkGameStatus(board, whiteToMove, &state)) {
            break;  // Game over
        }
        
        int startRow, startCol, endRow, endCol;
        char input[10];
        
        if (isBotTurn(gameMode, whiteToMove)) {
            // Bot's turn
            printf("Bot ready to move. Type 'next' to continue (or 'quit' to exit):\n");
            scanf("%9s", input);
            
            if (strcmp(input, "quit") == 0) break;
            if (strcmp(input, "next") != 0) {
                printf("Invalid command. Use 'next' to proceed or 'quit' to exit.\n");
                continue;
            }
            
            printf("Bot thinking...\n");
            selectBotMove(board, whiteToMove, &startRow, &startCol, &endRow, &endCol, &state);
            
            if (startRow == -1) {
                printf("No legal moves for bot. Game over?\n");
                break;
            }
            
            printf("Bot moves %c%d%c%d\n", 
                   'a' + startCol, 8 - startRow, 
                   'a' + endCol, 8 - endRow);
            
            executeMove(board, &state, startRow, startCol, endRow, endCol, 1);
            whiteToMove = !whiteToMove;
            
        } else {
            // Player's turn
            printf("Enter move or command (%s to move):\n", whiteToMove ? "White" : "Black");
            scanf("%9s", input);
            
            if (strcmp(input, "quit") == 0) break;
            if (handleModeChange(input, &gameMode)) continue;
            
            if (!parseMove(input, &startRow, &startCol, &endRow, &endCol)) {
                printf("Invalid input. Try again.\n");
                continue;
            }
            
            if (!isLegalMove(board, startRow, startCol, endRow, endCol, whiteToMove, &state)) {
                printf("Illegal move! Try again.\n");
                continue;
            }
            
            executeMove(board, &state, startRow, startCol, endRow, endCol, 0);
            whiteToMove = !whiteToMove;
        }
    }
    
    printf("Game ended.\n");
    return 0;
}