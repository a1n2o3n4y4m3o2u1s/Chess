# Chess Engine Examples

## Basic Usage

### Starting a Game

```bash
$ ./chess
Welcome to Chess Terminal. Commands:
- 'pvp' : Player vs Player
- 'pvb' : Player vs Bot
- 'bvb' : Bot vs Bot
- 'next' : Advance to next bot move (when bot is playing)
- 'quit' : Exit
- Move format: e2e4

Select game mode: pvp, pvb, bvb
pvp
Mode set to Player vs Player.
```

### Making Moves

```
8     r   n   b   q   k   b   n   r

7     p   p   p   p   p   p   p   p

6     .   .   .   .   .   .   .   .

5     .   .   .   .   .   .   .   .

4     .   .   .   .   .   .   .   .

3     .   .   .   .   .   .   .   .

2     P   P   P   P   P   P   P   P

1     R   N   B   Q   K   B   N   R


      a   b   c   d   e   f   g   h

Enter move or command (White to move):
e2e4
Move executed: P from e2 to e4
```

### Playing Against Bot

```
$ ./chess
Select game mode: pvp, pvb, bvb
pvb
Play as white or black? (enter 'white' or 'black')
white
Mode set to Player vs Bot. You play white.

[Board displayed]

Enter move or command (White to move):
e2e4
Move executed: P from e2 to e4

[Board displayed]

Bot ready to move. Type 'next' to continue (or 'quit' to exit):
next
Bot thinking...
Searching 3 moves deep...
Best move score: -15
Bot moves e7e5
Move executed: p from e7 to e5
```

## Programming Examples

### Example 1: Setting Up a Custom Position

```c
#include "board.h"
#include "game_state.h"

void setupCustomPosition(char board[8][8], GameState* state) {
    // Clear the board
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            board[row][col] = '.';
        }
    }
    
    // Set up a position: King and Rook endgame
    board[7][4] = 'K';  // White king on e1
    board[7][0] = 'R';  // White rook on a1
    board[0][4] = 'k';  // Black king on e8
    
    // Configure game state
    initializeGameState(state);
    state->whiteKingsideCastle = 0;  // Cannot castle
    state->whiteQueensideCastle = 0;
}
```

### Example 2: Analyzing a Position

```c
#include <stdio.h>
#include "board.h"
#include "moves.h"
#include "game_state.h"
#include "bot.h"

void analyzePosition(char board[8][8], GameState* state) {
    // Count material
    int whiteMaterial = 0, blackMaterial = 0;
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            char piece = board[row][col];
            int value = 0;
            
            switch (toupper(piece)) {
                case 'P': value = 1; break;
                case 'N': case 'B': value = 3; break;
                case 'R': value = 5; break;
                case 'Q': value = 9; break;
            }
            
            if (isWhitePiece(piece)) whiteMaterial += value;
            else if (isBlackPiece(piece)) blackMaterial += value;
        }
    }
    
    printf("Material balance: White %d - Black %d\n", 
           whiteMaterial, blackMaterial);
    
    // Count legal moves
    Move moves[4096];
    int whiteMovesCount = generateAllLegalMoves(board, 1, moves, state);
    int blackMovesCount = generateAllLegalMoves(board, 0, moves, state);
    
    printf("Mobility: White %d moves, Black %d moves\n",
           whiteMovesCount, blackMovesCount);
    
    // Check status
    printf("White in check: %s\n", 
           isKingInCheck(board, 1, state) ? "Yes" : "No");
    printf("Black in check: %s\n", 
           isKingInCheck(board, 0, state) ? "Yes" : "No");
}
```

### Example 3: Batch Game Analysis

```c
#include <stdio.h>
#include "board.h"
#include "moves.h"
#include "game_state.h"

typedef struct {
    char* moves[100];
    int count;
} GameRecord;

void replayGame(GameRecord* game) {
    char board[8][8];
    GameState state;
    
    initializeBoard(board);
    initializeGameState(&state);
    
    int whiteToMove = 1;
    
    for (int i = 0; i < game->count; i++) {
        char* moveStr = game->moves[i];
        
        // Parse move (e.g., "e2e4")
        int startCol = moveStr[0] - 'a';
        int startRow = 8 - (moveStr[1] - '0');
        int endCol = moveStr[2] - 'a';
        int endRow = 8 - (moveStr[3] - '0');
        
        // Validate and make move
        if (!isLegalMove(board, startRow, startCol, endRow, endCol, 
                         whiteToMove, &state)) {
            printf("Illegal move at move %d: %s\n", i + 1, moveStr);
            return;
        }
        
        // Execute move (simplified - full version in main.c)
        board[endRow][endCol] = board[startRow][startCol];
        board[startRow][startCol] = '.';
        
        whiteToMove = !whiteToMove;
        
        // Print position every 10 moves
        if ((i + 1) % 10 == 0) {
            printf("\nAfter move %d:\n", i + 1);
            printBoard(board);
        }
    }
}

int main() {
    GameRecord game;
    game.moves[0] = "e2e4";
    game.moves[1] = "e7e5";
    game.moves[2] = "g1f3";
    game.moves[3] = "b8c6";
    game.count = 4;
    
    replayGame(&game);
    return 0;
}
```

### Example 4: Custom AI Evaluation

```c
// Add to bot.c

static int evaluatePositionAdvanced(char board[8][8]) {
    int score = 0;
    
    // Material evaluation
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            char piece = board[row][col];
            if (isEmpty(piece)) continue;
            
            int materialValue = getMaterialValue(piece);
            int positionalBonus = 0;
            
            // Pawn structure bonuses
            if (toupper(piece) == 'P') {
                // Passed pawn bonus
                if (isPassedPawn(board, row, col)) {
                    positionalBonus += 50;
                }
                
                // Doubled pawn penalty
                if (isDoubledPawn(board, row, col)) {
                    positionalBonus -= 20;
                }
                
                // Isolated pawn penalty
                if (isIsolatedPawn(board, row, col)) {
                    positionalBonus -= 15;
                }
            }
            
            // Knight on rim is dim
            if (toupper(piece) == 'N') {
                if (col == 0 || col == 7 || row == 0 || row == 7) {
                    positionalBonus -= 10;
                }
            }
            
            // Rook on open file bonus
            if (toupper(piece) == 'R') {
                if (isOpenFile(board, col)) {
                    positionalBonus += 20;
                }
            }
            
            // King safety in middle game
            if (toupper(piece) == 'K' && !isEndgame(board)) {
                if (isKingSafe(board, row, col)) {
                    positionalBonus += 30;
                }
            }
            
            // Apply bonuses
            int totalValue = materialValue + positionalBonus;
            if (isWhitePiece(piece)) {
                score += totalValue;
            } else {
                score -= totalValue;
            }
        }
    }
    
    // Center control bonus
    score += evaluateCenterControl(board);
    
    // Mobility bonus
    score += evaluateMobility(board);
    
    return score;
}

// Helper functions
static int isPassedPawn(char board[8][8], int row, int col) {
    char pawn = board[row][col];
    int isWhite = isWhitePiece(pawn);
    int direction = isWhite ? -1 : 1;
    
    // Check if any enemy pawns can stop this pawn
    for (int r = row + direction; r >= 0 && r < 8; r += direction) {
        for (int c = col - 1; c <= col + 1; c++) {
            if (c < 0 || c > 7) continue;
            char piece = board[r][c];
            if (toupper(piece) == 'P' && isWhitePiece(piece) != isWhite) {
                return 0;  // Not passed
            }
        }
    }
    return 1;  // Passed pawn
}

static int evaluateCenterControl(char board[8][8]) {
    int score = 0;
    int centerSquares[][2] = {{3,3}, {3,4}, {4,3}, {4,4}};  // d4,e4,d5,e5
    
    for (int i = 0; i < 4; i++) {
        int row = centerSquares[i][0];
        int col = centerSquares[i][1];
        char piece = board[row][col];
        
        if (!isEmpty(piece)) {
            int bonus = 10;
            if (isWhitePiece(piece)) score += bonus;
            else score -= bonus;
        }
    }
    
    return score;
}
```

### Example 5: Move Suggestion System

```c
#include <stdio.h>
#include "board.h"
#include "moves.h"
#include "game_state.h"
#include "bot.h"

typedef struct {
    Move move;
    int score;
    char description[100];
} SuggestedMove;

void suggestMoves(char board[8][8], int whiteToMove, GameState* state, 
                  int numSuggestions) {
    Move allMoves[4096];
    int moveCount = generateAllLegalMoves(board, whiteToMove, allMoves, state);
    
    SuggestedMove suggestions[moveCount];
    
    // Evaluate each move
    for (int i = 0; i < moveCount; i++) {
        suggestions[i].move = allMoves[i];
        
        // Make move temporarily
        char tempStart = board[allMoves[i].startRow][allMoves[i].startCol];
        char tempEnd = board[allMoves[i].endRow][allMoves[i].endCol];
        
        board[allMoves[i].endRow][allMoves[i].endCol] = tempStart;
        board[allMoves[i].startRow][allMoves[i].startCol] = '.';
        
        // Simple evaluation (you can use minimax for better results)
        suggestions[i].score = evaluatePosition(board);
        if (!whiteToMove) suggestions[i].score = -suggestions[i].score;
        
        // Undo move
        board[allMoves[i].startRow][allMoves[i].startCol] = tempStart;
        board[allMoves[i].endRow][allMoves[i].endCol] = tempEnd;
        
        // Generate description
        sprintf(suggestions[i].description, "%c%d%c%d",
                'a' + allMoves[i].startCol, 8 - allMoves[i].startRow,
                'a' + allMoves[i].endCol, 8 - allMoves[i].endRow);
    }
    
    // Sort by score (bubble sort for simplicity)
    for (int i = 0; i < moveCount - 1; i++) {
        for (int j = 0; j < moveCount - i - 1; j++) {
            if (suggestions[j].score < suggestions[j + 1].score) {
                SuggestedMove temp = suggestions[j];
                suggestions[j] = suggestions[j + 1];
                suggestions[j + 1] = temp;
            }
        }
    }
    
    // Print top suggestions
    printf("\nTop %d suggested moves:\n", numSuggestions);
    for (int i = 0; i < numSuggestions && i < moveCount; i++) {
        printf("%d. %s (score: %d)\n", 
               i + 1, suggestions[i].description, suggestions[i].score);
    }
}
```

### Example 6: Puzzle Solver

```c
#include <stdio.h>
#include "board.h"
#include "moves.h"
#include "game_state.h"

void setupPuzzle(char board[8][8], GameState* state) {
    // Mate in 2 puzzle
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            board[i][j] = '.';
        }
    }
    
    // White to move and mate in 2
    board[0][7] = 'k';  // Black king on h8
    board[1][6] = 'p';  // Black pawn on g7
    board[2][5] = 'Q';  // White queen on f6
    board[7][7] = 'K';  // White king on h1
    
    initializeGameState(state);
}

int findMateInN(char board[8][8], GameState* state, int n, 
                int whiteToMove, Move* solution) {
    if (n == 0) {
        // Check if opponent is checkmated
        if (isKingInCheck(board, !whiteToMove, state) && 
            !hasAnyLegalMoves(board, !whiteToMove, state)) {
            return 1;  // Checkmate found
        }
        return 0;
    }
    
    Move moves[4096];
    int moveCount = generateAllLegalMoves(board, whiteToMove, moves, state);
    
    for (int i = 0; i < moveCount; i++) {
        // Save state
        char tempStart = board[moves[i].startRow][moves[i].startCol];
        char tempEnd = board[moves[i].endRow][moves[i].endCol];
        GameState tempState = *state;
        
        // Make move
        board[moves[i].endRow][moves[i].endCol] = tempStart;
        board[moves[i].startRow][moves[i].startCol] = '.';
        
        // Check if this leads to mate
        if (findMateInN(board, state, n - 1, !whiteToMove, solution)) {
            *solution = moves[i];
            
            // Undo move
            board[moves[i].startRow][moves[i].startCol] = tempStart;
            board[moves[i].endRow][moves[i].endCol] = tempEnd;
            *state = tempState;
            
            return 1;  // Solution found
        }
        
        // Undo move
        board[moves[i].startRow][moves[i].startCol] = tempStart;
        board[moves[i].endRow][moves[i].endCol] = tempEnd;
        *state = tempState;
    }
    
    return 0;  // No mate found
}

void solvePuzzle() {
    char board[8][8];
    GameState state;
    setupPuzzle(board, &state);
    
    printf("Puzzle position:\n");
    printBoard(board);
    
    Move solution;
    if (findMateInN(board, &state, 2, 1, &solution)) {
        printf("\nSolution found: %c%d%c%d\n",
               'a' + solution.startCol, 8 - solution.startRow,
               'a' + solution.endCol, 8 - solution.endRow);
    } else {
        printf("\nNo mate in 2 found.\n");
    }
}
```

### Example 7: Game Statistics

```c
#include <stdio.h>
#include "board.h"
#include "moves.h"
#include "game_state.h"

typedef struct {
    int totalMoves;
    int captures;
    int checks;
    int castles;
    int promotions;
    int enPassants;
} GameStatistics;

void initStats(GameStatistics* stats) {
    stats->totalMoves = 0;
    stats->captures = 0;
    stats->checks = 0;
    stats->castles = 0;
    stats->promotions = 0;
    stats->enPassants = 0;
}

void updateStats(char board[8][8], Move* move, GameState* state, 
                 int whiteToMove, GameStatistics* stats) {
    stats->totalMoves++;
    
    char piece = board[move->startRow][move->startCol];
    char target = board[move->endRow][move->endCol];
    
    // Capture
    if (!isEmpty(target)) {
        stats->captures++;
    }
    
    // En passant
    if (toupper(piece) == 'P' && move->endCol != move->startCol && isEmpty(target)) {
        stats->enPassants++;
        stats->captures++;
    }
    
    // Castling
    if (toupper(piece) == 'K' && abs(move->endCol - move->startCol) == 2) {
        stats->castles++;
    }
    
    // Promotion
    if (toupper(piece) == 'P' && (move->endRow == 0 || move->endRow == 7)) {
        stats->promotions++;
    }
    
    // Execute move (simplified)
    board[move->endRow][move->endCol] = piece;
    board[move->startRow][move->startCol] = '.';
    
    // Check
    if (isKingInCheck(board, !whiteToMove, state)) {
        stats->checks++;
    }
}

void printStats(GameStatistics* stats) {
    printf("\n=== Game Statistics ===\n");
    printf("Total moves: %d\n", stats->totalMoves);
    printf("Captures: %d\n", stats->captures);
    printf("Checks: %d\n", stats->checks);
    printf("Castles: %d\n", stats->castles);
    printf("Promotions: %d\n", stats->promotions);
    printf("En passants: %d\n", stats->enPassants);
}
```

### Example 8: Training Mode with Hints

```c
#include <stdio.h>
#include "board.h"
#include "moves.h"
#include "game_state.h"
#include "bot.h"

void trainingMode() {
    char board[8][8];
    GameState state;
    initializeBoard(board);
    initializeGameState(&state);
    
    int whiteToMove = 1;
    char input[10];
    
    printf("Training Mode - Type 'hint' for suggestions\n");
    
    while (1) {
        printBoard(board);
        
        if (checkGameStatus(board, whiteToMove, &state)) break;
        
        printf("Enter move (or 'hint' for help): ");
        scanf("%9s", input);
        
        if (strcmp(input, "hint") == 0) {
            // Get best move from bot
            int startRow, startCol, endRow, endCol;
            selectBotMove(board, whiteToMove, &startRow, &startCol, 
                         &endRow, &endCol, &state);
            
            printf("\nSuggested move: %c%d%c%d\n",
                   'a' + startCol, 8 - startRow,
                   'a' + endCol, 8 - endRow);
            
            // Explain why
            char piece = board[startRow][startCol];
            char target = board[endRow][endCol];
            
            printf("Explanation: ");
            if (!isEmpty(target)) {
                printf("Captures %c. ", target);
            }
            if (toupper(piece) == 'P' && (endRow == 0 || endRow == 7)) {
                printf("Promotes pawn. ");
            }
            printf("\n");
            continue;
        }
        
        if (strcmp(input, "quit") == 0) break;
        
        // Parse and validate move
        if (strlen(input) != 4) {
            printf("Invalid format. Use: e2e4\n");
            continue;
        }
        
        int startCol = input[0] - 'a';
        int startRow = 8 - (input[1] - '0');
        int endCol = input[2] - 'a';
        int endRow = 8 - (input[3] - '0');
        
        if (!isLegalMove(board, startRow, startCol, endRow, endCol, 
                        whiteToMove, &state)) {
            printf("Illegal move! Try again or type 'hint'.\n");
            continue;
        }
        
        // Execute move
        board[endRow][endCol] = board[startRow][startCol];
        board[startRow][startCol] = '.';
        
        whiteToMove = !whiteToMove;
    }
}
```

### Example 9: Position Evaluation Breakdown

```c
#include <stdio.h>
#include "board.h"
#include "bot.h"

void explainEvaluation(char board[8][8]) {
    int whiteMaterial = 0, blackMaterial = 0;
    int whitePosition = 0, blackPosition = 0;
    int whiteMobility = 0, blackMobility = 0;
    
    // Material count
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            char piece = board[row][col];
            if (isEmpty(piece)) continue;
            
            int value = 0;
            switch (toupper(piece)) {
                case 'P': value = 100; break;
                case 'N': value = 320; break;
                case 'B': value = 330; break;
                case 'R': value = 500; break;
                case 'Q': value = 900; break;
            }
            
            if (isWhitePiece(piece)) {
                whiteMaterial += value;
            } else {
                blackMaterial += value;
            }
        }
    }
    
    printf("\n=== Position Evaluation ===\n");
    printf("Material:\n");
    printf("  White: %d centipawns\n", whiteMaterial);
    printf("  Black: %d centipawns\n", blackMaterial);
    printf("  Balance: %+d (White's favor)\n", whiteMaterial - blackMaterial);
    
    // Overall evaluation
    int totalScore = evaluatePosition(board);
    printf("\nTotal evaluation: %+d centipawns\n", totalScore);
    
    if (totalScore > 200) {
        printf("Assessment: White is winning\n");
    } else if (totalScore < -200) {
        printf("Assessment: Black is winning\n");
    } else if (totalScore > 50) {
        printf("Assessment: White is slightly better\n");
    } else if (totalScore < -50) {
        printf("Assessment: Black is slightly better\n");
    } else {
        printf("Assessment: Position is equal\n");
    }
}
```

### Example 10: Tournament System

```c
#include <stdio.h>
#include <time.h>
#include "board.h"
#include "moves.h"
#include "game_state.h"
#include "bot.h"

typedef struct {
    char name[50];
    int wins;
    int losses;
    int draws;
    int searchDepth;
} Player;

int playGame(Player* white, Player* black) {
    char board[8][8];
    GameState state;
    initializeBoard(board);
    initializeGameState(&state);
    
    int whiteToMove = 1;
    int moveCount = 0;
    int maxMoves = 100;  // Draw after 100 moves
    
    while (moveCount < maxMoves) {
        if (checkGameStatus(board, whiteToMove, &state)) {
            if (isKingInCheck(board, whiteToMove, &state)) {
                return whiteToMove ? -1 : 1;  // -1 = black wins, 1 = white wins
            } else {
                return 0;  // Stalemate = draw
            }
        }
        
        int startRow, startCol, endRow, endCol;
        
        // Set appropriate search depth
        if (whiteToMove) {
            setBotDepth(white->searchDepth);
        } else {
            setBotDepth(black->searchDepth);
        }
        
        selectBotMove(board, whiteToMove, &startRow, &startCol, 
                     &endRow, &endCol, &state);
        
        // Execute move
        board[endRow][endCol] = board[startRow][startCol];
        board[startRow][startCol] = '.';
        
        whiteToMove = !whiteToMove;
        moveCount++;
    }
    
    return 0;  // Draw by move limit
}

void runTournament(Player players[], int numPlayers, int gamesPerPair) {
    printf("\n=== TOURNAMENT START ===\n");
    printf("Players: %d\n", numPlayers);
    printf("Games per pairing: %d\n\n", gamesPerPair);
    
    // Round-robin tournament
    for (int i = 0; i < numPlayers; i++) {
        for (int j = i + 1; j < numPlayers; j++) {
            printf("Match: %s vs %s\n", players[i].name, players[j].name);
            
            for (int game = 0; game < gamesPerPair; game++) {
                // Alternate colors
                Player* white = (game % 2 == 0) ? &players[i] : &players[j];
                Player* black = (game % 2 == 0) ? &players[j] : &players[i];
                
                int result = playGame(white, black);
                
                if (result == 1) {
                    white->wins++;
                    black->losses++;
                    printf("  Game %d: %s wins\n", game + 1, white->name);
                } else if (result == -1) {
                    black->wins++;
                    white->losses++;
                    printf("  Game %d: %s wins\n", game + 1, black->name);
                } else {
                    white->draws++;
                    black->draws++;
                    printf("  Game %d: Draw\n", game + 1);
                }
            }
            printf("\n");
        }
    }
    
    // Print final standings
    printf("=== FINAL STANDINGS ===\n");
    for (int i = 0; i < numPlayers; i++) {
        float score = players[i].wins + 0.5 * players[i].draws;
        printf("%s (Depth %d): %.1f points (%d-%d-%d)\n",
               players[i].name, players[i].searchDepth,
               score, players[i].wins, players[i].draws, players[i].losses);
    }
}

int main() {
    Player players[4];
    
    strcpy(players[0].name, "Beginner Bot");
    players[0].searchDepth = 2;
    players[0].wins = players[0].losses = players[0].draws = 0;
    
    strcpy(players[1].name, "Intermediate Bot");
    players[1].searchDepth = 4;
    players[1].wins = players[1].losses = players[1].draws = 0;
    
    strcpy(players[2].name, "Advanced Bot");
    players[2].searchDepth = 6;
    players[2].wins = players[2].losses = players[2].draws = 0;
    
    strcpy(players[3].name, "Expert Bot");
    players[3].searchDepth = 8;
    players[3].wins = players[3].losses = players[3].draws = 0;
    
    runTournament(players, 4, 2);  // 2 games per pairing
    
    return 0;
}
```

## Command Line Usage Examples

### Basic Commands

```bash
# Compile
make

# Run game
./chess

# Run tests
make test

# Clean build files
make clean

# Rebuild from scratch
make rebuild
```

### Game Session Examples

**Quick game (Player vs Player):**

```
$ ./chess
pvp
e2e4
e7e5
g1f3
b8c6
quit
```

**Watch bots play:**

```
$ ./chess
bvb
next
next
next
# ... continue typing 'next' to watch the game
```

**Play as black:**

```
$ ./chess
pvb
black
next
# Bot makes first move as white
e7e5
# Your move
next
# Bot responds
```

## Integration Examples

### Using the Engine in Another Program

```c
// my_chess_app.c
#include "board.h"
#include "moves.h"
#include "game_state.h"
#include "bot.h"

int main() {
    // Initialize
    char board[8][8];
    GameState state;
    initializeBoard(board);
    initializeGameState(&state);
    
    // Your custom game loop
    while (1) {
        // Your UI code here
        
        // Get user input
        int startRow, startCol, endRow, endCol;
        getUserInput(&startRow, &startCol, &endRow, &endCol);
        
        // Validate move
        if (isLegalMove(board, startRow, startCol, endRow, endCol, 1, &state)) {
            // Execute move
            board[endRow][endCol] = board[startRow][startCol];
            board[startRow][startCol] = '.';
            
            // Update display
            updateDisplay(board);
        }
    }
    
    return 0;
}
```

### Compile with your program:

```bash
gcc -o my_app my_chess_app.c board.c moves.c game_state.c bot.c -Wall -O2
```

## Debugging Examples

### Enable Verbose Output

```c
// Add to bot.c
#define DEBUG_MODE 1

void selectBotMove(...) {
    #ifdef DEBUG_MODE
    printf("Evaluating %d moves...\n", numMoves);
    #endif
    
    for (int i = 0; i < numMoves; i++) {
        // ... evaluation code ...
        
        #ifdef DEBUG_MODE
        printf("Move %c%d%c%d: score = %d\n",
               'a' + moves[i].startCol, 8 - moves[i].startRow,
               'a' + moves[i].endCol, 8 - moves[i].endRow,
               score);
        #endif
    }
}
```

### Memory Debugging with Valgrind

```bash
# Compile with debug symbols
gcc -g -o chess main.c board.c moves.c game_state.c bot.c

# Run with valgrind
valgrind --leak-check=full ./chess
```

### GDB Debugging

```bash
# Compile with debug symbols
gcc -g -o chess main.c board.c moves.c game_state.c bot.c

# Run in GDB
gdb ./chess

# GDB commands:
(gdb) break isLegalMove
(gdb) run
(gdb) print board
(gdb) step
(gdb) continue
```

## Performance Examples

### Benchmarking Move Generation

```c
#include <time.h>

void benchmarkMoveGeneration() {
    char board[8][8];
    GameState state;
    initializeBoard(board);
    initializeGameState(&state);
    
    Move moves[4096];
    int iterations = 10000;
    
    clock_t start = clock();
    for (int i = 0; i < iterations; i++) {
        generateAllLegalMoves(board, 1, moves, &state);
    }
    clock_t end = clock();
    
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Generated moves %d times in %.3f seconds\n", iterations, time_taken);
    printf("Average: %.2f microseconds per generation\n", 
           (time_taken / iterations) * 1000000);
}
```

### Bot Performance at Different Depths

```c
void benchmarkBotDepths() {
    char board[8][8];
    GameState state;
    initializeBoard(board);
    initializeGameState(&state);
    
    for (int depth = 2; depth <= 8; depth += 2) {
        setBotDepth(depth);
        
        int startRow, startCol, endRow, endCol;
        
        clock_t start = clock();
        selectBotMove(board, 1, &startRow, &startCol, &endRow, &endCol, &state);
        clock_t end = clock();
        
        double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
        printf("Depth %d: %.3f seconds\n", depth, time_taken);
    }
}
```

This comprehensive examples file should help users understand how to use and extend the chess engine!