CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I.
LDFLAGS = -L. -lraylib -lm -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL

# Source files
SRCS = src/main.c src/globals.c src/world.c src/projectiles.c src/player.c src/monsters.c src/ui.c src/game.c
OBJS = $(SRCS:.c=.o)
TARGET = gridlock-arena

# Default target
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Compile source files to object files
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJS) $(TARGET)

# Run the game
run: $(TARGET)
	./$(TARGET)

# Rebuild everything
rebuild: clean all

.PHONY: all clean run rebuild
