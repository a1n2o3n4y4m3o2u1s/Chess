#include "timeControl.h"
#include <stdio.h>
#include <math.h>
#include <time.h>

void initTimeControl(TimeControl* tc, double baseMinutes, double incrementSeconds) {
    double baseSeconds = baseMinutes * 60.0;
    tc->whiteTimeRemaining = baseSeconds;
    tc->blackTimeRemaining = baseSeconds;
    tc->increment = incrementSeconds;
    tc->enabled = (baseMinutes > 0) ? 1 : 0;
}

time_t startMoveTimer() {
    return time(NULL);  // Use real time instead of CPU time
}

void endMoveTimer(TimeControl* tc, int whiteToMove, time_t startTime) {
    if (!tc->enabled) return;
    
    time_t endTime = time(NULL);
    double elapsed = difftime(endTime, startTime);  // This gives real seconds
    
    if (whiteToMove) {
        tc->whiteTimeRemaining -= elapsed;
        tc->whiteTimeRemaining += tc->increment;
    } else {
        tc->blackTimeRemaining -= elapsed;
        tc->blackTimeRemaining += tc->increment;
    }
    
    // Debug output to verify it's working
    printf("Time used: %.1f seconds\n", elapsed);
}

int hasTimeExpired(TimeControl* tc, int whiteToMove) {
    if (!tc->enabled) return 0;
    
    double timeLeft = whiteToMove ? tc->whiteTimeRemaining : tc->blackTimeRemaining;
    return timeLeft <= 0.0;
}

void formatTime(double seconds, char* buffer, int bufferSize) {
    if (seconds < 0) seconds = 0;
    
    int minutes = (int)(seconds / 60);
    double secs = seconds - (minutes * 60);
    
    snprintf(buffer, bufferSize, "%d:%04.1f", minutes, secs);
}

void displayTime(TimeControl* tc) {
    if (!tc->enabled) return;
    
    char whiteTime[20], blackTime[20];
    formatTime(tc->whiteTimeRemaining, whiteTime, sizeof(whiteTime));
    formatTime(tc->blackTimeRemaining, blackTime, sizeof(blackTime));
    
    printf("Time - White: %s | Black: %s\n", whiteTime, blackTime);
}

double calculateBotThinkTime(TimeControl* tc, int whiteToMove, int positionEval, int moveNumber) {
    if (!tc->enabled) {
        // No time control - use default
        return 10.0;
    }
    
    double timeRemaining = whiteToMove ? tc->whiteTimeRemaining : tc->blackTimeRemaining;
    
    // Emergency time - use increment only
    if (timeRemaining < tc->increment * 2) {
        return fmin(tc->increment * 0.8, timeRemaining * 0.5);
    }
    
    // Estimate remaining moves in game (simple heuristic)
    int estimatedMovesRemaining = 40;
    if (moveNumber > 20) {
        estimatedMovesRemaining = 30;
    }
    if (moveNumber > 40) {
        estimatedMovesRemaining = 20;
    }
    
    // Base allocation: divide remaining time by estimated moves
    double baseTime = timeRemaining / estimatedMovesRemaining;
    
    // Add most of the increment (save a bit for safety)
    baseTime += tc->increment * 0.9;
    
    // Position complexity modifier
    // If position is critical (eval close to 0 or very high), think longer
    double complexityMultiplier = 1.0;
    int absEval = abs(positionEval);
    
    if (absEval < 100) {
        // Very equal position - think more
        complexityMultiplier = 1.3;
    } else if (absEval < 300) {
        // Slightly unequal - normal thinking
        complexityMultiplier = 1.1;
    } else if (absEval > 500) {
        // Winning/losing position - can think less
        complexityMultiplier = 0.8;
    }
    
    baseTime *= complexityMultiplier;
    
    // Never use more than 1/10 of remaining time (safety)
    double maxTime = timeRemaining * 0.10;
    baseTime = fmin(baseTime, maxTime);
    
    // Never use less than half the increment (always make progress)
    double minTime = tc->increment * 0.5;
    baseTime = fmax(baseTime, minTime);
    
    // Absolute minimum to avoid instant moves
    baseTime = fmax(baseTime, 0.5);
    
    return baseTime;
}