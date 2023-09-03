# Define the compiler and compiler flags
CC = gcc
CFLAGS = -Wall -Iinclude
MD = mkdir -p

# Define source file directories
SRC_DIR = .
BIN_DIR = bin

# List of source files and corresponding executables
SOURCES = $(wildcard $(SRC_DIR)/*.c)
EXECUTABLES = $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/%,$(SOURCES))

# Default target to build all executables
all: $(BIN_DIR) $(EXECUTABLES)

# Rule to create the bin directory
$(BIN_DIR):
	$(MD) $(BIN_DIR)

# Rule to build an executable from a C source file
$(BIN_DIR)/%: $(SRC_DIR)/%.c	
	$(CC) $(CFLAGS) $< -o $@

# Clean target to remove all executables and the bin directory
clean:
	rm -rf $(BIN_DIR) output.txt

run:
	./bin/app-test
