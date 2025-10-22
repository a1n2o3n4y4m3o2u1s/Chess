# Chess Terminal Game

A complete chess implementation in C with AI opponent using minimax algorithm with alpha-beta pruning.

## Features

- ✅ Full chess rules implementation (castling, en passant, pawn promotion)
- ✅ Check, checkmate, and stalemate detection
- ✅ Player vs Player mode
- ✅ Player vs Bot mode (play as white or black)
- ✅ Bot vs Bot mode (watch AI play itself)
- ✅ Configurable AI difficulty (search depth)

## Project Structure

```
chess/
├── board.h           - Board representation and display
├── board.c
├── moves.h           - Move validation and piece logic
├── moves.c
├── game_state.h      - Game state and check detection
├── game_state.c
├── bot.h             - AI move selection
├── bot.c
├── main.c            - Main game loop and UI
├── Makefile          - Build system
└── README.md         - This file
```

## Module Overview

### board.h/c

- **Purpose**: Board display and basic piece identification
- **Key Functions**:
  - `initializeBoard()` - Sets up starting position
  - `printBoard()` - Displays current board state
  - `isWhitePiece()`, `isBlackPiece()`, `isEmpty()` - Piece type checking

### moves.h/c

- **Purpose**: Move validation and piece movement rules
- **Key Functions**:
  - `isLegalMove()` - Complete move validation
  - `isValidPawnMove()`, `isValidKnightMove()`, etc. - Piece-specific rules
  - `generateAllLegalMoves()` - For AI move generation
  - `canPieceMoveTo()` - Simplified validation for attack detection

### game_state.h/c

- **Purpose**: Game state management and check detection
- **Key Structures**:
  - `GameState` - Tracks castling rights and en passant
- **Key Functions**:
  - `isKingInCheck()` - Check detection
  - `isSquareAttacked()` - Attack detection for any square
  - `checkGameStatus()` - Detects checkmate/stalemate
  - `hasAnyLegalMoves()` - Verifies legal moves exist

### bot.h/c

- **Purpose**: AI opponent implementation
- **Key Functions**:
  - `selectBotMove()` - Chooses best move using minimax
  - `evaluatePosition()` - Scores board positions
  - `minimax()` - Recursive search with alpha-beta pruning
  - `setBotDepth()` - Configure AI strength

### main.c

- **Purpose**: Game loop and user interaction
- **Key Functions**:
  - `main()` - Main game loop
  - `executeMove()` - Handles move execution and special cases
  - `selectGameMode()` - Mode selection interface
  - `parseMove()` - Converts algebraic notation to coordinates

## Building and Running

### Compile

```bash
make
```

### Run

```bash
./chess
```

### Clean build artifacts

```bash
make clean
```

### Rebuild from scratch

```bash
make rebuild
```

## Usage

### Starting the Game

When you start the game, select a mode:

- `pvp` - Player vs Player
- `pvb` - Player vs Bot (then choose ‘white’ or ‘black’)
- `bvb` - Bot vs Bot

### Making Moves

Enter moves in algebraic notation: `e2e4`

- First two characters: starting square (file + rank)
- Last two characters: destination square (file + rank)
- Example: `e2e4` moves piece from e2 to e4

### Bot vs Bot Mode

- Type `next` to advance to the next move
- Useful for watching the AI play against itself

### In-Game Commands

- `quit` - Exit the game
- `pvp`, `pvb`, `bvb` - Switch game modes mid-game
- `next` - Advance bot move (in bot vs bot mode)

### Pawn Promotion

When a pawn reaches the opposite end:

- Human players: Choose Q/R/B/N when prompted
- Bot: Automatically promotes to Queen

## AI Configuration

### Adjusting Bot Strength

In `bot.c`, modify the `BOT_SEARCH_DEPTH` variable:

```c
static int BOT_SEARCH_DEPTH = 6;  // Default: looks 3 moves ahead
```

**Recommended depths:**

- `2` - Very weak (0.5 moves ahead) - instant moves
- `4` - Beginner (1 move ahead) - very fast
- `6` - Intermediate (3 moves ahead) - good balance
- `8` - Advanced (4 moves ahead) - slower but stronger
- `10+` - Expert (5+ moves ahead) - very slow

Higher depths make the bot stronger but significantly slower.

### Evaluation Function

The bot uses material-based evaluation:

- Pawn: 100 points
- Knight: 320 points
- Bishop: 330 points
- Rook: 500 points
- Queen: 900 points
- King: 20,000 points

You can modify `evaluatePosition()` in `bot.c` to add positional evaluation.

## Code Architecture

### Move Validation Flow

```
User Input → parseMove()
           ↓
isLegalMove() checks:
  1. Correct color moving?
  2. Not capturing own piece?
  3. Piece-specific rules (isValidPawnMove, etc.)
  4. Move doesn't leave king in check?
           ↓
executeMove() handles:
  1. En passant capture
  2. Moving the piece
  3. Castling (moving rook)
  4. Pawn promotion
  5. Update castling rights
  6. Update en passant state
```

### Bot Move Selection Flow

```
generateAllLegalMoves()
         ↓
For each move:
  makeMove()
         ↓
  minimax() - recursive search
         ↓
  unmakeMove()
         ↓
Select move with best score
```

### Check Detection Flow

```
isKingInCheck()
     ↓
Find king position
     ↓
isSquareAttacked() by opponent?
     ↓
Scan all opponent pieces
     ↓
canPieceMoveTo() king's square?
```

## Chess Rules Implemented

### Basic Rules

- ✅ All piece movements (pawn, knight, bishop, rook, queen, king)
- ✅ Captures
- ✅ Turn-based play

### Special Moves

- ✅ Castling (kingside and queenside)
- ✅ En passant capture
- ✅ Pawn promotion

### Game Ending Conditions

- ✅ Checkmate
- ✅ Stalemate
- ✅ Check detection and display

### Castling Rules

- King and rook haven’t moved
- No pieces between king and rook
- King not in check
- King doesn’t pass through check
- King doesn’t end in check

### En Passant Rules

- Opponent pawn moves two squares forward
- Your pawn is beside it
- Must capture immediately (next move)

## Extending the Code

### Adding New Features

**1. Add a new piece:**

- Add movement logic in `moves.c`
- Update `canPieceMoveTo()` switch statement
- Add to evaluation in `evaluatePosition()`

**2. Improve AI evaluation:**

- Modify `evaluatePosition()` in `bot.c`
- Add positional bonuses (center control, king safety, etc.)

**3. Add move history:**

- Create history structure in `game_state.h`
- Track moves in `executeMove()`
- Add undo functionality

**4. Save/load games:**

- Implement FEN (Forsyth-Edwards Notation) parser
- Save game state to file
- Load game state from file

**5. Add time controls:**

- Track time per player
- Add clock display
- Implement time-based loss condition

## Performance Notes

- Board representation: 8x8 array (simple and fast)
- Move generation: ~64-200 legal moves per position typically
- Search performance: ~1000-100000 nodes/second depending on depth
- Memory usage: Minimal (stack-based search)

## Debugging Tips

**Print current game state:**

```c
printf("En passant: col=%d row=%d\n", state.enPassantCol, state.enPassantRow);
printf("White castling: K=%d Q=%d\n", state.whiteKingsideCastle, state.whiteQueensideCastle);
```

**Test specific positions:**

- Modify `initializeBoard()` to set up test positions
- Use FEN strings for complex positions (requires FEN parser)

**Debug move generation:**

```c
Move moves[4096];
int count = generateAllLegalMoves(board, 1, moves, &state);
printf("Generated %d legal moves\n", count);
```

## Known Limitations

- No threefold repetition detection
- No fifty-move rule
- No insufficient material detection
- Basic material-only evaluation (no positional understanding)
- Single-threaded (no parallel search)

## Future Enhancements

- [ ] Opening book
- [ ] Endgame tablebase
- [ ] UCI protocol support
- [ ] PGN game notation
- [ ] Graphical interface
- [ ] Network play
- [ ] Move hints
- [ ] Analysis mode

## License

Free to use and modify.

## Credits

Chess rules: FIDE (World Chess Federation)
Algorithm: Minimax with alpha-beta pruning (John von Neumann, 1928)