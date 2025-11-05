# Chess Engine with AI Bot

A sophisticated chess implementation featuring an advanced AI bot with iterative deepening, transposition tables, and competitive time management. Built in C with modular architecture for extensible chess AI development.

## Features

• **Advanced Bot** with iterative deepening alpha-beta search  
• **Multiple Game Modes**: Player vs Player, Player vs Bot, Bot vs Bot  
• **Competitive Time Control** with increment and dynamic thinking time  
• **Transposition Tables** for efficient move caching and pruning  
• **Move Ordering** with killer moves and history heuristics  
• **Position Evaluation** with material, positional, and tactical scoring  
• **Complete Chess Rules**: castling, en passant, pawn promotion, check/checkmate detection  
• **Flexible Bot Configuration**: adjustable thinking time and automation modes  

## Project Structure

```
chess/
├── main.c                 # Main game loop and user interface
├── board.c/h             # Board representation and display
├── moves.c/h             # Move generation and validation
├── gameState.c/h         # Game state tracking
├── timeControl.c/h       # Time management and bot thinking time
├── evaluation.c/h        # Position evaluation
├── bot/                  # AI bot components
│   ├── bot.c/h           # Bot move selection and iterative deepening
│   ├── search.c/h        # Alpha-beta search implementation
│   ├── transposition.c/h # Hash tables for position caching
│   ├── moveOrdering.c/h  # Move ordering heuristics
│   └── evaluation.c/h    # Bot-specific evaluation functions
├── Makefile              # Build configuration
└── README.md             # Project documentation
```

## Installation & Compilation

1. **Clone and compile**:
   ```bash
   git clone https://github.com/yourusername/chess-engine.git
   cd chess-engine
   make
   ```

2. **Run the game**:
   ```bash
   ./chess
   ```

## Game Modes

- **Player vs Player (pvp)**: Two human players
- **Player vs Bot (pvb)**: Play against the AI as white or black
- **Bot vs Bot (bvb)**: Watch AI play against itself

## Time Control Options

- **With Time Control**: Set base time + increment (e.g., 5+3 for 5 minutes + 3 seconds per move)
- **Without Time Control**: Set custom bot thinking time (default configurable)

## Bot Commands

- **auto**: Bot moves automatically
- **manual**: Type 'next' to advance bot moves
- **quit**: Exit the game

## Move Input Format

Use algebraic notation: `e2e4` (from e2 to e4)

## How the AI Works

1. **Iterative Deepening**: Searches progressively deeper positions
2. **Alpha-Beta Pruning**: Eliminates irrelevant branches from search tree
3. **Transposition Tables**: Caches previously evaluated positions
4. **Move Ordering**: Prioritizes promising moves using killer moves and history heuristics
5. **Time Management**: Dynamically allocates thinking time based on position complexity
6. **Position Evaluation**: Scores positions using material, mobility, pawn structure, and king safety

## Key Algorithms

- **Minimax with Alpha-Beta Pruning** for efficient game tree search
- **Zobrist Hashing** for transposition table keys
- **Quiescence Search** to avoid horizon effect
- **Principal Variation Search** for better move ordering

## Technical Details

- **Transposition Table**: Default 16MB hash table
- **Move Generation**: Legal move generation with full chess rules

## Development

**Build**:
```bash
make clean && make
```

**Debug Build**:
```bash
make CFLAGS="-Wall -Wextra -g -I. -Ibot"
```

## Dependencies

- Standard C library (stdlib, stdio, string, time, math)
- No external dependencies required

## License

This project is licensed under the MIT License.  
See the [LICENSE](./LICENSE) file for details.

## AI Disclaimer

Portions of this codebase and documentation were developed with AI assistance. The core chess algorithms and engine architecture represent original implementation work.
