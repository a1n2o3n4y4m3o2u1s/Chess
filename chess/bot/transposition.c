#include <transposition.h>
#include <stdlib.h>
#include <string.h>  // For memset, but calloc initializes to zero
#include <moves.h>

// Zobrist hashing for position identification
static unsigned long long zobristTable[MAX_BOARD_SIZE][MAX_BOARD_SIZE][MAX_PIECE_TYPES];
static int zobristInitialized = 0;

static TTEntry* transpositionTable = NULL;
static int transpositionTableInitialized = 0;

// Initialize Zobrist random numbers
void initZobrist(void) {
    if (zobristInitialized) return;
    
    srand(INITIAL_SEED);  // Fixed seed for consistency
    for (int row = 0; row < MAX_BOARD_SIZE; row++) {
        for (int col = 0; col < MAX_BOARD_SIZE; col++) {
            for (int piece = 0; piece < MAX_PIECE_TYPES; piece++) {
                zobristTable[row][col][piece] = 
                    ((unsigned long long)rand() << 32) | rand();
            }
        }
    }
    zobristInitialized = 1;
}

// Map piece character to index (0-11)
int pieceToIndex(char piece) {
    const char* pieces = "PNBRQKpnbrqk";
    for (int i = 0; i < MAX_PIECE_TYPES; i++) {
        if (pieces[i] == piece) return i;
    }
    return -1;
}

// Compute hash for current board position
unsigned long long computeHash(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE]) {
    unsigned long long hash = 0;
    for (int row = 0; row < MAX_BOARD_SIZE; row++) {
        for (int col = 0; col < MAX_BOARD_SIZE; col++) {
            if (!isEmpty(board[row][col])) {
                int idx = pieceToIndex(board[row][col]);
                if (idx >= 0) {
                    hash ^= zobristTable[row][col][idx];
                }
            }
        }
    }
    return hash;
}

// Initialize transposition table
int initTranspositionTable(void) {
    if (transpositionTable == NULL) {
        transpositionTable = (TTEntry*)calloc(TT_SIZE, sizeof(TTEntry));
        if (transpositionTable == NULL) {
            return 0; // Allocation failed
        }
        transpositionTableInitialized = 1;
    }
    initZobrist();
    return 1; // Success
}

// Free transposition table
void freeTranspositionTable(void) {
    if (transpositionTable != NULL) {
        free(transpositionTable);
        transpositionTable = NULL;
        transpositionTableInitialized = 0;
    }
}

// Probe transposition table
TTEntry* probeTranspositionTable(unsigned long long hash) {
    if (!transpositionTableInitialized) return NULL;
    
    int index = hash % TT_SIZE;
    TTEntry* entry = &transpositionTable[index];
    if (entry->hash == hash) {
        return entry;
    }
    return NULL;
}

// Store in transposition table
void storeTranspositionTable(unsigned long long hash, int depth, int score, 
                             int flag, Move* bestMove) {
    if (!transpositionTableInitialized) return;
    
    int index = hash % TT_SIZE;
    TTEntry* entry = &transpositionTable[index];
    
    // Replace if this is a deeper search or empty slot
    if (entry->hash == 0 || entry->depth <= depth) {
        entry->hash = hash;
        entry->depth = depth;
        entry->score = score;
        entry->flag = flag;
        if (bestMove) {
            entry->bestMove = *bestMove;
        }
    }
}