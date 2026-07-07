# 1. Compiler and Flags
CC       = gcc
CFLAGS   = -Wall -Wextra -std=c11 -O2 -Iinclude
TARGET   = losephus.exe
LIBS     = -luser32 -ldwmapi -luuid -lole32 
RC	 = windres

# 2. Directories
SRC_DIR  = src
OBJ_DIR  = build

# 3. Source and Object Files
# Finds all .c files inside the src/ directory
SRCS     = $(wildcard $(SRC_DIR)/*.c)
# Maps src/main.c to build/main.o
OBJS     = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))
# Compiled resource file path
RES_OBJ = $(OBJ_DIR)/resources.o

# 4. Default Rule
all: $(TARGET)

# 5. Link Object Files AND the Resource Object into the executable
$(TARGET): $(OBJS) $(RES_OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(RES_OBJ) $(LIBS)

# 6. Compile Source Files into Object Files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@if not exist $(OBJ_DIR) mkdir $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 7. Compile Windows Resource Files (.rc) into a COFF Object File
$(RES_OBJ): resources.rc app.manifest
	@if not exist $(OBJ_DIR) mkdir $(OBJ_DIR)
	$(RC) resources.rc -O coff -o $(RES_OBJ)

# 8. Clean up Build Files (Windows Friendly)
clean:
	@if exist $(TARGET) del /Q $(TARGET)
	@if exist $(OBJ_DIR) rmdir /S /Q $(OBJ_DIR)

.PHONY: all clean
