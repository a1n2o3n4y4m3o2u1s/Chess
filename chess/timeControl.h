#ifndef TIME_CONTROL_H
#define TIME_CONTROL_H

#include <time.h>
#include "bot/bot.h"  // ADD THIS to get BotSettings from bot.h

// Time control configuration
typedef struct {
    double whiteTimeRemaining;    // seconds
    double blackTimeRemaining;    // seconds
    double increment;             // seconds added after each move
    int enabled;                  // 0 = no time control, 1 = time control enabled
} TimeControl;

// REMOVE the BotSettings definition from here - it's now in bot/bot.h

// Initialize time control with base time and increment
void initTimeControl(TimeControl* tc, double baseMinutes, double incrementSeconds);

// Start timing a move
time_t startMoveTimer();

// End timing and update time remaining
void endMoveTimer(TimeControl* tc, int whiteToMove, time_t startTime);

// Check if player has run out of time
int hasTimeExpired(TimeControl* tc, int whiteToMove);

// Display current time status
void displayTime(TimeControl* tc);

// Calculate how much time the bot should use for this move
// Considers: time remaining, increment, position evaluation, move number
// Enhanced competitive version with phase-aware allocation and safety margins
double calculateBotThinkTime(TimeControl* tc, int whiteToMove, int positionEval, int moveNumber);

// Format seconds into MM:SS.d format
void formatTime(double seconds, char* buffer, int bufferSize);

#endif