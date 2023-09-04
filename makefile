# Define commands & the compiler and compiler flags
CC = gcc
CFLAGS = -Wall -Iinclude
MD = mkdir -p

# Define source file directories
SRC_DIR = ./src
BIN_DIR = bin

# List of source files and corresponding executables
SOURCES = $(wildcard $(SRC_DIR)/*.c)
EXECUTABLES = $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/%,$(SOURCES))

# Build all executables
all: $(BIN_DIR) $(EXECUTABLES)

# Create the bin directory
$(BIN_DIR):
	$(MD) $(BIN_DIR)

# Build an executable from a C source file
$(BIN_DIR)/%: $(SRC_DIR)/%.c	
	$(CC) $(CFLAGS) $< -o $@

# Clean target to remove all executables, generated files and the bin directory
clean:
	rm -rf $(BIN_DIR) output.txt

run_app:
	./bin/app files/*.txt

run_slave:
	./bin/worker files/file1.txt

# run_view: 
