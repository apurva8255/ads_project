# ============================================================================
#  Emergency Response Dispatch & Optimal Routing System — Makefile
# ============================================================================

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -Iinclude
LDFLAGS = -lm

SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj

# Collect all .c files in src/
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

TARGET = emergency_system

# ── Default target ───────────────────────────────────────────────────────
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# ── Clean ────────────────────────────────────────────────────────────────
clean:
	rm -rf $(OBJ_DIR) $(TARGET) $(TARGET).exe

# ── Run ──────────────────────────────────────────────────────────────────
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
