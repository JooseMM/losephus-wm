# 1. Compiler and Flags
CC       = gcc
CFLAGS   = -Wall -Wextra -std=c11 -O2 -Iinclude
TARGET   = program.exe
LIBS     = -luser32 -ldwmapi -lole32

# 2. Directories
SRC_DIR  = src
OBJ_DIR  = build

# 3. Source and Object Files
# Finds all .c files inside the src/ directory
SRCS     = $(wildcard $(SRC_DIR)/*.c)
# Maps src/main.c to build/main.o
OBJS     = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# 4. Default Rule
all: $(TARGET)

# 5. Link Object Files from the build directory into the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

# 6. Compile Source Files into Object Files
# The '@if not exist' line automatically creates the build directory on Windows
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@if not exist $(OBJ_DIR) mkdir $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 7. Clean up Build Files (Windows Friendly)
clean:
	@if exist $(TARGET) del /Q $(TARGET)
	@if exist $(OBJ_DIR) rmdir /S /Q $(OBJ_DIR)

.PHONY: all clean
