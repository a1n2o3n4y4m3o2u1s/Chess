CC = gcc
# Add current directory and bot subdirectory to include path
CFLAGS = -Wall -Wextra -O2 -I. -Ibot
TARGET = chess
TEST_TARGET = test_chess

# Source files
SRCS = main.c board.c moves.c gameState.c timeControl.c bot/bot.c bot/transposition.c bot/evaluation.c bot/moveOrdering.c bot/search.c
OBJS = $(SRCS:.c=.o)

# Test files
TEST_SRCS = test.c board.c moves.c gameState.c timeControl.c
TEST_OBJS = $(TEST_SRCS:.c=.o)

# Header files
HEADERS = board.h moves.h gameState.h timeControl.h bot/bot.h bot/transposition.h bot/evaluation.h bot/moveOrdering.h bot/search.h

# Default target
all: $(TARGET)

# Link object files to create executable - ADDED -lm HERE
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) -lm

# Test executable - ADDED -lm HERE
$(TEST_TARGET): $(TEST_OBJS)
	$(CC) $(CFLAGS) -o $(TEST_TARGET) $(TEST_OBJS) -lm

# Compile source files to object files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Run tests
test: $(TEST_TARGET)
	./$(TEST_TARGET)

# Clean build artifacts
clean:
	rm -f $(OBJS) $(TEST_OBJS) $(TARGET) $(TEST_TARGET)

# Rebuild everything
rebuild: clean all

.PHONY: all test clean rebuild