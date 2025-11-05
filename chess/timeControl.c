#include "timeControl.h"
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>

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
        // CHANGED: This should never be called when no time control is enabled
        // Main.c should handle the default thinking time instead
        return 2.0; // Fallback, but main.c should provide the configured default
    }
    
    double timeRemaining = whiteToMove ? tc->whiteTimeRemaining : tc->blackTimeRemaining;
    
    // Safety margin - always keep at least 2 increments as emergency reserve
    double safetyMargin = tc->increment * 2.0;
    double usableTime = timeRemaining - safetyMargin;
    
    // If we're in serious time trouble, use only safe portion of increment
    if (usableTime <= 0) {
        return fmin(tc->increment * 0.7, timeRemaining * 0.3);
    }
    
    // More sophisticated move phase detection
    double phaseMultiplier;
    if (moveNumber < 15) {
        phaseMultiplier = 1.4; // Opening - think more for development
    } else if (moveNumber < 30) {
        phaseMultiplier = 1.2; // Early middlegame
    } else if (moveNumber < 45) {
        phaseMultiplier = 1.0; // Middlegame
    } else {
        phaseMultiplier = 0.8; // Endgame - can think faster
    }
    
    // Estimate remaining moves based on game phase
    int estimatedMovesRemaining;
    if (moveNumber < 20) {
        estimatedMovesRemaining = 50 - moveNumber;
    } else if (moveNumber < 40) {
        estimatedMovesRemaining = 60 - moveNumber; // Middlegame can be longer
    } else {
        estimatedMovesRemaining = 80 - moveNumber; // Endgames can have many moves
    }
    estimatedMovesRemaining = fmax(estimatedMovesRemaining, 10); // At least 10 moves
    
    // Base time allocation using non-linear formula (more aggressive early, conservative late)
    double baseTime = (usableTime / estimatedMovesRemaining) * phaseMultiplier;
    
    // Position criticality analysis - more nuanced
    double criticalityMultiplier = 1.0;
    int absEval = abs(positionEval);
    
    if (absEval < 50) {
        // Highly critical position - nearly equal
        criticalityMultiplier = 1.6;
    } else if (absEval < 150) {
        // Moderately critical
        criticalityMultiplier = 1.3;
    } else if (absEval < 300) {
        // Slight advantage
        criticalityMultiplier = 1.1;
    } else if (absEval > 600) {
        // Clear advantage - think less unless near winning blow
        criticalityMultiplier = 0.6;
    } else if (absEval > 1000) {
        // Winning position - minimal thinking
        criticalityMultiplier = 0.4;
    }
    
    baseTime *= criticalityMultiplier;
    
    // Add increment with phase-dependent usage
    double incrementUsage;
    if (moveNumber < 10) {
        incrementUsage = 0.3; // Use less increment in opening
    } else if (timeRemaining > tc->increment * 10) {
        incrementUsage = 0.9; // Healthy time - use most of increment
    } else {
        incrementUsage = 0.5; // Moderate time - use half increment
    }
    baseTime += tc->increment * incrementUsage;
    
    // Dynamic time bounds based on game phase and time situation
    double maxTime;
    if (moveNumber < 10) {
        maxTime = timeRemaining * 0.15; // Can use more in opening
    } else if (timeRemaining > 180) { // More than 3 minutes
        maxTime = timeRemaining * 0.12;
    } else if (timeRemaining > 60) { // More than 1 minute
        maxTime = timeRemaining * 0.10;
    } else {
        maxTime = timeRemaining * 0.08; // Less when time is short
    }
    
    double minTime = fmax(tc->increment * 0.4, 0.5); // At least half increment or 0.5s
    
    // Apply bounds
    baseTime = fmin(baseTime, maxTime);
    baseTime = fmax(baseTime, minTime);
    
    // Sudden death protection - never drop below safe threshold
    if (tc->increment == 0 && timeRemaining < 30) {
        baseTime = fmin(baseTime, timeRemaining * 0.2);
    }
    
    // Ensure we don't exceed remaining time (safety)
    baseTime = fmin(baseTime, timeRemaining * 0.95);
    
    return baseTime;
}