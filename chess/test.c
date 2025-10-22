#include <stdio.h>
#include <string.h>
#include <board.h>
#include <moves.h>
#include <gameState.h>

// Test counter
static int tests_passed = 0;
static int tests_failed = 0;

// Test assertion macro
#define ASSERT(condition, test_name) \
    if (condition) { \
        printf("✓ PASS: %s\n", test_name); \
        tests_passed++; \
    } else { \
        printf("✗ FAIL: %s\n", test_name); \
        tests_failed++; \
    }

// ============================================================================
// TEST HELPER FUNCTIONS
// ============================================================================

void setupEmptyBoard(char board[8][8]) {
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            board[row][col] = '.';
        }
    }
}

// ============================================================================
// BOARD TESTS
// ============================================================================

void testBoardInitialization() {
    printf("\n=== Board Initialization Tests ===\n");
    
    char board[8][8];
    initializeBoard(board);
    
    ASSERT(board[0][0] == 'r', "Black rook on a8");
    ASSERT(board[0][4] == 'k', "Black king on e8");
    ASSERT(board[1][0] == 'p', "Black pawn on a7");
    ASSERT(board[6][0] == 'P', "White pawn on a2");
    ASSERT(board[7][4] == 'K', "White king on e1");
    ASSERT(board[7][7] == 'R', "White rook on h1");
    ASSERT(board[3][3] == '.', "Empty square on d5");
}

void testPieceIdentification() {
    printf("\n=== Piece Identification Tests ===\n");
    
    ASSERT(isWhitePiece('K') == 1, "K is white piece");
    ASSERT(isWhitePiece('P') == 1, "P is white piece");
    ASSERT(isWhitePiece('k') == 0, "k is not white piece");
    ASSERT(isBlackPiece('k') == 1, "k is black piece");
    ASSERT(isBlackPiece('p') == 1, "p is black piece");
    ASSERT(isBlackPiece('K') == 0, "K is not black piece");
    ASSERT(isEmpty('.') == 1, ". is empty");
    ASSERT(isEmpty('P') == 0, "P is not empty");
}

// ============================================================================
// PAWN MOVEMENT TESTS
// ============================================================================

void testPawnMovement() {
    printf("\n=== Pawn Movement Tests ===\n");
    
    char board[8][8];
    GameState state;
    initializeGameState(&state);
    setupEmptyBoard(board);
    
    // White pawn on e2
    board[6][4] = 'P';
    
    ASSERT(isValidPawnMove(board, 6, 4, 5, 4, &state) == 1, "White pawn single move forward");
    ASSERT(isValidPawnMove(board, 6, 4, 4, 4, &state) == 1, "White pawn double move from start");
    ASSERT(isValidPawnMove(board, 6, 4, 3, 4, &state) == 0, "White pawn cannot move 3 squares");
    ASSERT(isValidPawnMove(board, 6, 4, 5, 3, &state) == 0, "White pawn cannot capture empty diagonal");
    
    // Test capture
    board[5][3] = 'p';  // Black pawn on d3
    ASSERT(isValidPawnMove(board, 6, 4, 5, 3, &state) == 1, "White pawn can capture diagonally");
    
    // Test en passant
    state.enPassantCol = 5;
    state.enPassantRow = 5;
    board[6][5] = 'p';  // Black pawn beside white pawn
    ASSERT(isValidPawnMove(board, 6, 4, 5, 5, &state) == 1, "White pawn en passant capture");
}

// ============================================================================
// KNIGHT MOVEMENT TESTS
// ============================================================================

void testKnightMovement() {
    printf("\n=== Knight Movement Tests ===\n");
    
    ASSERT(isValidKnightMove(4, 4, 6, 5) == 1, "Knight L-shape: 2 down, 1 right");
    ASSERT(isValidKnightMove(4, 4, 6, 3) == 1, "Knight L-shape: 2 down, 1 left");
    ASSERT(isValidKnightMove(4, 4, 2, 5) == 1, "Knight L-shape: 2 up, 1 right");
    ASSERT(isValidKnightMove(4, 4, 5, 6) == 1, "Knight L-shape: 1 down, 2 right");
    ASSERT(isValidKnightMove(4, 4, 5, 5) == 0, "Knight cannot move diagonally 1 square");
    ASSERT(isValidKnightMove(4, 4, 6, 4) == 0, "Knight cannot move straight 2 squares");
    ASSERT(isValidKnightMove(4, 4, 7, 7) == 0, "Knight cannot move far diagonally");
}

// ============================================================================
// BISHOP MOVEMENT TESTS
// ============================================================================

void testBishopMovement() {
    printf("\n=== Bishop Movement Tests ===\n");
    
    char board[8][8];
    setupEmptyBoard(board);
    board[4][4] = 'B';  // White bishop on e4
    
    ASSERT(isValidBishopMove(board, 4, 4, 1, 1) == 1, "Bishop diagonal: up-left");
    ASSERT(isValidBishopMove(board, 4, 4, 7, 7) == 1, "Bishop diagonal: down-right");
    ASSERT(isValidBishopMove(board, 4, 4, 2, 6) == 1, "Bishop diagonal: up-right");
    ASSERT(isValidBishopMove(board, 4, 4, 4, 7) == 0, "Bishop cannot move horizontally");
    ASSERT(isValidBishopMove(board, 4, 4, 7, 4) == 0, "Bishop cannot move vertically");
    
    // Test blocked path
    board[3][3] = 'P';  // Block diagonal
    ASSERT(isValidBishopMove(board, 4, 4, 2, 2) == 0, "Bishop blocked by own piece");
}

// ============================================================================
// ROOK MOVEMENT TESTS
// ============================================================================

void testRookMovement() {
    printf("\n=== Rook Movement Tests ===\n");
    
    char board[8][8];
    setupEmptyBoard(board);
    board[4][4] = 'R';  // White rook on e4
    
    ASSERT(isValidRookMove(board, 4, 4, 4, 0) == 1, "Rook horizontal: left");
    ASSERT(isValidRookMove(board, 4, 4, 4, 7) == 1, "Rook horizontal: right");
    ASSERT(isValidRookMove(board, 4, 4, 0, 4) == 1, "Rook vertical: up");
    ASSERT(isValidRookMove(board, 4, 4, 7, 4) == 1, "Rook vertical: down");
    ASSERT(isValidRookMove(board, 4, 4, 6, 6) == 0, "Rook cannot move diagonally");
    
    // Test blocked path
    board[4][2] = 'P';  // Block horizontal
    ASSERT(isValidRookMove(board, 4, 4, 4, 0) == 0, "Rook blocked horizontally");
}

// ============================================================================
// QUEEN MOVEMENT TESTS
// ============================================================================

void testQueenMovement() {
    printf("\n=== Queen Movement Tests ===\n");
    
    char board[8][8];
    setupEmptyBoard(board);
    board[4][4] = 'Q';  // White queen on e4
    
    ASSERT(isValidQueenMove(board, 4, 4, 4, 0) == 1, "Queen moves like rook horizontally");
    ASSERT(isValidQueenMove(board, 4, 4, 0, 4) == 1, "Queen moves like rook vertically");
    ASSERT(isValidQueenMove(board, 4, 4, 7, 7) == 1, "Queen moves like bishop diagonally");
    ASSERT(isValidQueenMove(board, 4, 4, 1, 1) == 1, "Queen moves like bishop diagonally");
}

// ============================================================================
// KING MOVEMENT TESTS
// ============================================================================

void testKingMovement() {
    printf("\n=== King Movement Tests ===\n");
    
    char board[8][8];
    GameState state;
    initializeGameState(&state);
    setupEmptyBoard(board);
    board[4][4] = 'K';  // White king on e4
    
    ASSERT(isValidKingMove(board, 4, 4, 5, 4, &state) == 1, "King moves 1 square down");
    ASSERT(isValidKingMove(board, 4, 4, 3, 4, &state) == 1, "King moves 1 square up");
    ASSERT(isValidKingMove(board, 4, 4, 4, 5, &state) == 1, "King moves 1 square right");
    ASSERT(isValidKingMove(board, 4, 4, 5, 5, &state) == 1, "King moves 1 square diagonally");
    ASSERT(isValidKingMove(board, 4, 4, 6, 4, &state) == 0, "King cannot move 2 squares (not castling)");
}

// ============================================================================
// CASTLING TESTS
// ============================================================================

void testCastling() {
    printf("\n=== Castling Tests ===\n");
    
    char board[8][8];
    GameState state;
    initializeGameState(&state);
    setupEmptyBoard(board);
    
    // Setup white kingside castling
    board[7][4] = 'K';  // King on e1
    board[7][7] = 'R';  // Rook on h1
    
    ASSERT(isValidKingMove(board, 7, 4, 7, 6, &state) == 1, "White kingside castling allowed");
    
    // Test castling rights
    state.whiteKingsideCastle = 0;
    ASSERT(isValidKingMove(board, 7, 4, 7, 6, &state) == 0, "Castling not allowed when right lost");
}

// ============================================================================
// CHECK DETECTION TESTS
// ============================================================================

void testCheckDetection() {
    printf("\n=== Check Detection Tests ===\n");
    
    char board[8][8];
    GameState state;
    initializeGameState(&state);
    setupEmptyBoard(board);
    
    // White king on e1, black rook on e8
    board[7][4] = 'K';
    board[0][4] = 'r';
    
    ASSERT(isKingInCheck(board, 1, &state) == 1, "White king in check from rook");
    
    // Remove rook
    board[0][4] = '.';
    ASSERT(isKingInCheck(board, 1, &state) == 0, "White king not in check");
    
    // Add black queen on a5
    board[3][0] = 'q';
    ASSERT(isKingInCheck(board, 1, &state) == 0, "Queen too far away");
    
    // Move queen to e4
    board[3][0] = '.';
    board[4][4] = 'q';
    ASSERT(isKingInCheck(board, 1, &state) == 1, "White king in check from queen");
}

// ============================================================================
// MOVE LEGALITY TESTS
// ============================================================================

void testMoveLegality() {
    printf("\n=== Move Legality Tests ===\n");
    
    char board[8][8];
    GameState state;
    initializeBoard(board);
    initializeGameState(&state);
    
    // Standard opening moves
    ASSERT(isLegalMove(board, 6, 4, 5, 4, 1, &state) == 1, "e2-e3 legal for white");
    ASSERT(isLegalMove(board, 6, 4, 4, 4, 1, &state) == 1, "e2-e4 legal for white");
    ASSERT(isLegalMove(board, 1, 4, 2, 4, 0, &state) == 1, "e7-e6 legal for black");
    
    // Illegal moves
    ASSERT(isLegalMove(board, 6, 4, 4, 4, 0, &state) == 0, "Black cannot move white pawn");
    ASSERT(isLegalMove(board, 7, 1, 5, 0, 1, &state) == 1, "Knight can jump over pieces");
    ASSERT(isLegalMove(board, 7, 5, 5, 3, 1, &state) == 0, "Bishop blocked by pawn");
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main() {
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║   CHESS ENGINE UNIT TEST SUITE         ║\n");
    printf("╚════════════════════════════════════════╝\n");
    
    testBoardInitialization();
    testPieceIdentification();
    testPawnMovement();
    testKnightMovement();
    testBishopMovement();
    testRookMovement();
    testQueenMovement();
    testKingMovement();
    testCastling();
    testCheckDetection();
    testMoveLegality();
    
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║   TEST RESULTS                         ║\n");
    printf("╠════════════════════════════════════════╣\n");
    printf("║   Tests Passed: %-4d                   ║\n", tests_passed);
    printf("║   Tests Failed: %-4d                   ║\n", tests_failed);
    printf("║   Total Tests:  %-4d                   ║\n", tests_passed + tests_failed);
    printf("╚════════════════════════════════════════╝\n");
    printf("\n");
    
    if (tests_failed == 0) {
        printf("✓ ALL TESTS PASSED!\n\n");
        return 0;
    } else {
        printf("✗ SOME TESTS FAILED\n\n");
        return 1;
    }
}