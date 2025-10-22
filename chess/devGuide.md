# Developer Guide

## Quick Start

```bash
# Compile the game
make

# Run the game
./chess

# Run unit tests
make test

# Clean and rebuild
make rebuild
```

## Code Organization

### Module Dependencies

```
main.c
  ├─→ board.h (display)
  ├─→ moves.h (validation)
  ├─→ game_state.h (check detection)
  └─→ bot.h (AI)

bot.c
  ├─→ board.h
  ├─→ moves.h
  └─→ game_state.h

game_state.c
  ├─→ board.h
  └─→ moves.h

moves.c
  ├─→ board.h
  └─→ game_state.h

board.c
  └─→ (no dependencies)
```

**Design principle**: Lower-level modules (board) don’t depend on higher-level modules (bot).

## Key Data Structures

### Board Representation

```c
char board[8][8];
// [0][0] = a8 (top-left)
// [7][7] = h1 (bottom-right)
// Uppercase = white, lowercase = black, '.' = empty
```

### GameState

```c
typedef struct {
    int whiteKingsideCastle;   // Can white castle O-O?
    int whiteQueensideCastle;  // Can white castle O-O-O?
    int blackKingsideCastle;   // Can black castle O-O?
    int blackQueensideCastle;  // Can black castle O-O-O?
    int enPassantCol;          // Column for en passant (-1 if none)
    int enPassantRow;          // Row for en passant
} GameState;
```

### Move

```c
typedef struct {
    int startRow;
    int startCol;
    int endRow;
    int endCol;
} Move;
```

## Function Call Chains

### Making a Move (Player)

```
main()
  → parseMove()              // Convert "e2e4" to coordinates
  → isLegalMove()            // Validate the move
      → isCorrectColorMoving()
      → isNotCapturingSameColor()
      → canPieceMoveTo()     // Piece-specific rules
      → doesMovePutKingInCheck()
  → executeMove()            // Apply the move
      → handleEnPassantCapture()
      → handleCastling()
      → handlePawnPromotion()
      → updateCastlingRights()
      → updateEnPassantState()
```

### Bot Move Selection

```
selectBotMove()
  → generateAllLegalMoves()  // Get all possible moves
  → For each move:
      → makeMove()           // Try the move
      → minimax()            // Evaluate recursively
          → generateAllLegalMoves()
          → For each response:
              → makeMove()
              → minimax() [recursive]
              → unmakeMove()
          → evaluatePosition()  // Score the position
      → unmakeMove()           // Undo the move
  → Select best move
```

### Check Detection

```
isKingInCheck()
  → Find king position
  → isSquareAttacked()
      → For each opponent piece:
          → canPieceMoveTo() king's square?
```

## Common Tasks

### Adding a New Piece Type

1. **Add movement logic** in `moves.c`:

```c
int isValidDragonMove(char board[8][8], int startRow, int startCol, 
                      int endRow, int endCol) {
    // Implement dragon movement rules
    return 1;
}
```

1. **Update `canPieceMoveTo()`** in `moves.c`:

```c
switch (toupper(piece)) {
    case 'D': return isValidDragonMove(board, startRow, startCol, endRow, endCol);
    // ... other cases
}
```

1. **Add to evaluation** in `bot.c`:

```c
pieceValues['D'] = 1200;  // Dragon worth 12 pawns
pieceValues['d'] = 1200;
```

1. **Update board initialization** if needed in `board.c`.

### Improving AI Evaluation

**Current evaluation** (material only):

```c
static int evaluatePosition(char board[8][8]) {
    int score = 0;
    // Count piece values
    return score;
}
```

**Enhanced evaluation** (add positional bonuses):

```c
static int evaluatePosition(char board[8][8]) {
    int score = 0;
    
    // Material
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            char piece = board[row][col];
            score += getMaterialValue(piece);
            score += getPositionalBonus(piece, row, col);
        }
    }
    
    // Center control
    score += evaluateCenterControl(board);
    
    // King safety
    score += evaluateKingSafety(board);
    
    return score;
}
```

**Example positional bonus**:

```c
int getPositionalBonus(char piece, int row, int col) {
    if (toupper(piece) != 'P') return 0;
    
    // Bonus for advanced pawns
    int rank = isWhitePiece(piece) ? (7 - row) : row;
    return rank * 10;  // 10 points per rank advanced
}
```

### Adding Move History

1. **Define history structure** in `game_state.h`:

```c
typedef struct {
    Move move;
    char capturedPiece;
    GameState stateBefore;
} MoveHistory;

typedef struct {
    MoveHistory moves[512];
    int count;
} GameHistory;
```

1. **Track moves** in `main.c`:

```c
static GameHistory history;

void recordMove(Move* move, char captured, GameState* state) {
    history.moves[history.count].move = *move;
    history.moves[history.count].capturedPiece = captured;
    history.moves[history.count].stateBefore = *state;
    history.count++;
}
```

1. **Implement undo**:

```c
void undoLastMove(char board[8][8], GameState* state, GameHistory* history) {
    if (history->count == 0) return;
    
    history->count--;
    MoveHistory* last = &history->moves[history->count];
    
    // Restore board
    board[last->move.startRow][last->move.startCol] = 
        board[last->move.endRow][last->move.endCol];
    board[last->move.endRow][last->move.endCol] = last->capturedPiece;
    
    // Restore state
    *state = last->stateBefore;
}
```

### Implementing FEN Support

FEN (Forsyth-Edwards Notation) is the standard for describing chess positions.

**Example FEN**: `rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1`

**Parser** (add to `board.c`):

```c
int loadFromFEN(char board[8][8], GameState* state, const char* fen) {
    int row = 0, col = 0;
    
    // Parse board position
    while (*fen && *fen != ' ') {
        if (*fen == '/') {
            row++;
            col = 0;
        } else if (*fen >= '1' && *fen <= '8') {
            int empty = *fen - '0';
            for (int i = 0; i < empty; i++) {
                board[row][col++] = '.';
            }
        } else {
            board[row][col++] = *fen;
        }
        fen++;
    }
    
    // Skip space
    fen++;
    
    // Active color (w/b)
    int whiteToMove = (*fen == 'w') ? 1 : 0;
    fen += 2;
    
    // Castling rights
    state->whiteKingsideCastle = (strchr(fen, 'K') != NULL);
    state->whiteQueensideCastle = (strchr(fen, 'Q') != NULL);
    state->blackKingsideCastle = (strchr(fen, 'k') != NULL);
    state->blackQueensideCastle = (strchr(fen, 'q') != NULL);
    
    // ... parse en passant, halfmove, fullmove
    
    return whiteToMove;
}
```

### Adding Time Controls

1. **Add timing structure** in `game_state.h`:

```c
#include <time.h>

typedef struct {
    time_t whiteTimeRemaining;  // Seconds
    time_t blackTimeRemaining;
    time_t moveStartTime;
} GameClock;
```

1. **Track time** in `main.c`:

```c
GameClock clock;
clock.whiteTimeRemaining = 600;  // 10 minutes
clock.blackTimeRemaining = 600;

// Before move
clock.moveStartTime = time(NULL);

// After move
time_t elapsed = time(NULL) - clock.moveStartTime;
if (whiteToMove) {
    clock.whiteTimeRemaining -= elapsed;
} else {
    clock.blackTimeRemaining -= elapsed;
}

// Check for time loss
if (clock.whiteTimeRemaining <= 0) {
    printf("Black wins on time!\n");
}
```

## Performance Optimization

### Current Performance Bottlenecks

1. **Move Generation**: O(64 × 64) = 4096 checks per position
1. **Check Detection**: O(64) pieces × O(64) squares
1. **Minimax Search**: Exponential in depth

### Optimization Strategies

#### 1. Move Ordering

```c
int scoreMoveForOrdering(char board[8][8], Move* move) {
    int score = 0;
    
    // Captures first (MVV-LVA: Most Valuable Victim - Least Valuable Attacker)
    char victim = board[move->endRow][move->endCol];
    if (!isEmpty(victim)) {
        score += 10000 + getMaterialValue(victim);
    }
    
    // Checks
    // Promotions
    // etc.
    
    return score;
}

void sortMoves(Move moves[], int count, char board[8][8]) {
    // Sort by score (captures first improves alpha-beta pruning)
}
```

#### 2. Transposition Table

```c
#define TABLE_SIZE 1000000

typedef struct {
    uint64_t hash;      // Zobrist hash
    int depth;
    int score;
    int flag;           // EXACT, LOWER_BOUND, UPPER_BOUND
} TTEntry;

TTEntry transpositionTable[TABLE_SIZE];

int probeTable(uint64_t hash, int depth, int alpha, int beta) {
    // Check if position already evaluated
}
```

#### 3. Iterative Deepening

```c
void selectBotMove(...) {
    int bestMove = -1;
    
    // Search incrementally deeper
    for (int depth = 1; depth <= BOT_SEARCH_DEPTH; depth++) {
        for (int i = 0; i < numMoves; i++) {
            int score = minimax(board, state, depth, -999999, 999999, !whiteToMove);
            // Update best move
        }
    }
}
```

#### 4. Bitboards (Advanced)

Replace `char board[8][8]` with bitboards for faster operations:

```c
typedef struct {
    uint64_t whitePawns;
    uint64_t whiteKnights;
    // ... etc
} Bitboard;

// Check if square is attacked (bit manipulation)
int isSquareAttacked(Bitboard* bb, int square) {
    uint64_t attacks = getKnightAttacks(square);
    if (attacks & bb->blackKnights) return 1;
    // ... much faster
}
```

## Debugging Tips

### Print Board State

```c
void debugPrintBoard(char board[8][8]) {
    printf("\n");
    for (int r = 0; r < 8; r++) {
        printf("%d ", 8 - r);
        for (int c = 0; c < 8; c++) {
            printf("[%c]", board[r][c]);
        }
        printf("\n");
    }
    printf("   a  b  c  d  e  f  g  h\n");
}
```

### Print Game State

```c
void debugPrintState(GameState* state) {
    printf("Castling: W(K=%d Q=%d) B(K=%d Q=%d)\n",
        state->whiteKingsideCastle, state->whiteQueensideCastle,
        state->blackKingsideCastle, state->blackQueensideCastle);
    printf("En passant: col=%d row=%d\n", 
        state->enPassantCol, state->enPassantRow);
}
```

### Print Move Generation

```c
void debugPrintMoves(Move moves[], int count) {
    printf("Generated %d moves:\n", count);
    for (int i = 0; i < count; i++) {
        printf("  %c%d%c%d\n",
            'a' + moves[i].startCol, 8 - moves[i].startRow,
            'a' + moves[i].endCol, 8 - moves[i].endRow);
    }
}
```

### Validate Move Consistency

```c
void validateMove(char board[8][8], Move* move, GameState* state) {
    // Before move
    char piece = board[move->startRow][move->startCol];
    char target = board[move->endRow][move->endCol];
    
    printf("Moving %c from %c%d to %c%d (target: %c)\n",
        piece,
        'a' + move->startCol, 8 - move->startRow,
        'a' + move->endCol, 8 - move->endRow,
        target);
    
    // Verify legal
    if (!isLegalMove(board, move->startRow, move->startCol, 
                     move->endRow, move->endCol, 
                     isWhitePiece(piece), state)) {
        printf("ERROR: Invalid move detected!\n");
    }
}
```

## Testing Strategy

### Unit Tests

Run existing tests:

```bash
make test
```

### Add New Tests

In `test.c`:

```c
void testMyNewFeature() {
    printf("\n=== My New Feature Tests ===\n");
    
    char board[8][8];
    GameState state;
    setupTestPosition(board, &state);
    
    ASSERT(myFunction() == expected, "Test description");
}
```

### Integration Testing

Test full games:

```c
void testScholarsMate() {
    char board[8][8];
    GameState state;
    initializeBoard(board);
    initializeGameState(&state);
    
    // Play moves
    makeTestMove(board, &state, "e2e4");
    makeTestMove(board, &state, "e7e5");
    makeTestMove(board, &state, "f1c4");
    makeTestMove(board, &state, "b8c6");
    makeTestMove(board, &state, "d1h5");
    makeTestMove(board, &state, "g8f6");
    makeTestMove(board, &state, "h5f7");  // Checkmate
    
    ASSERT(isKingInCheck(board, 0, &state) == 1, "Black in check");
    ASSERT(!hasAnyLegalMoves(board, 0, &state), "Black has no moves");
}
```

### Perft Testing

Test move generation correctness:

```c
uint64_t perft(char board[8][8], GameState* state, int depth, int whiteToMove) {
    if (depth == 0) return 1;
    
    Move moves[4096];
    int numMoves = generateAllLegalMoves(board, whiteToMove, moves, state);
    
    uint64_t nodes = 0;
    for (int i = 0; i < numMoves; i++) {
        // Make move
        nodes += perft(board, state, depth - 1, !whiteToMove);
        // Unmake move
    }
    return nodes;
}

// Starting position should give:
// Depth 1: 20 nodes
// Depth 2: 400 nodes
// Depth 3: 8,902 nodes
// Depth 4: 197,281 nodes
```

## Code Style Guidelines

### Naming Conventions

- **Functions**: `camelCase` or `snake_case` (be consistent)
- **Structures**: `PascalCase`
- **Constants**: `UPPER_SNAKE_CASE`
- **Local variables**: `camelCase`

### Comments

```c
// Single line comment for brief explanations

/*
 * Multi-line comment for:
 * - Function documentation
 * - Complex algorithms
 * - Important notes
 */
```

### Function Documentation

```c
/**
 * Checks if a move is legal according to all chess rules.
 *
 * @param board     The current board state
 * @param startRow  Starting row (0-7)
 * @param startCol  Starting column (0-7)
 * @param endRow    Ending row (0-7)
 * @param endCol    Ending column (0-7)
 * @param whiteToMove  1 if white's turn, 0 if black's turn
 * @param state     Current game state (castling, en passant)
 * @return 1 if move is legal, 0 otherwise
 */
int isLegalMove(char board[8][8], int startRow, int startCol, 
                int endRow, int endCol, int whiteToMove, GameState* state);
```

## Git Workflow

### Branching Strategy

```bash
main              # Stable, tested code
├─ feature/ai-improvement
├─ feature/fen-parser
└─ bugfix/castling-check
```

### Commit Messages

```
feat: Add transposition table for faster search
fix: Correct en passant capture logic
docs: Update developer guide with optimization tips
test: Add perft testing for move generation
refactor: Simplify bishop movement validation
```

## Resources

### Chess Programming

- [Chess Programming Wiki](https://www.chessprogramming.org/)
- [Minimax Algorithm](https://en.wikipedia.org/wiki/Minimax)
- [Alpha-Beta Pruning](https://en.wikipedia.org/wiki/Alpha%E2%80%93beta_pruning)

### Testing Positions

- [Perft Results](https://www.chessprogramming.org/Perft_Results)
- [Test Positions](https://www.chessprogramming.org/Test-Positions)

### Rules Reference

- [FIDE Laws of Chess](https://www.fide.com/laws-of-chess)

## Common Pitfalls

1. **En Passant Timing**: Must be used immediately, expires after one turn
1. **Castling Through Check**: King cannot pass through attacked squares
1. **Check Validation**: Must check if move leaves own king in check
1. **Coordinate System**: Row 0 = rank 8, Row 7 = rank 1
1. **State Management**: Save and restore state when trying moves in minimax