# Chess Engine with AI Bot

A sophisticated chess implementation featuring an advanced AI bot with iterative deepening, transposition tables, and competitive time management. Built in C with modular architecture for extensible chess AI development.

## Features

• **Advanced AI Bot** with iterative deepening alpha-beta search  
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
- **time**: Display remaining time
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

## Example Game Session

```
=== Time Control Setup ===
Enter base time in minutes (0 for no time control): 0
No time control (unlimited time)
Enter default bot thinking time in seconds: 3.0

Bot play mode:
  'auto' - Bot moves automatically
  'manual' - Type 'next' to advance bot moves
Select mode: auto

=== Bot Thinking ===
Allocated time: 3.0 seconds
Position eval: 25
Legal moves: 20
Depth 1: score=   35, nodes=    45, time=0.02s
Depth 2: score=   28, nodes=   892, time=0.15s
Depth 3: score=   31, nodes=  4521, time=0.87s
```

## Configuration

- Adjust `BOT_TIME_LIMIT_SECONDS` in `bot.c` for default thinking time
- Modify evaluation weights in `evaluation.c` for different playing styles
- Tune search parameters in `search.c` for strength/speed balance

## Technical Details

- **Search Depth**: Typically reaches 4-8 ply in middlegame positions
- **Nodes per Second**: 5,000-50,000 depending on position complexity
- **Transposition Table**: Default 16MB hash table
- **Move Generation**: Legal move generation with full chess rules

## Development

**Build**:
```bash
make clean && make
```

**Run Tests**:
```bash
make test
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
