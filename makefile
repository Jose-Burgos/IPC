# Define commands & the compiler and compiler flags
CC = gcc
CFLAGS = -Iinclude -lpthread -g -Wall -Wextra -Werror -lrt
MD = mkdir -p

# Define source file directories
SRC_DIR = ./src
BIN_DIR = bin

# List of source files, TAD files, and corresponding executables
MAIN_SOURCES = ./src/app.c ./src/view.c ./src/worker.c
LIB_SRC = ./src/master.c
EXECUTABLES = $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/%,$(MAIN_SOURCES))

# Build all executables
all: $(BIN_DIR) $(EXECUTABLES)

# Create the bin directory
$(BIN_DIR):
	$(MD) $(BIN_DIR)

# Build an executable from a C source file
$(BIN_DIR)/%: $(SRC_DIR)/%.c $(LIB_SRC)
	$(CC) $(CFLAGS) $< $(LIB_SRC) -o $@

# Clean target to remove all executables, generated files, and the bin directory
clean:
	rm -rf $(BIN_DIR) output.txt

run_app:
	./bin/app ./files/*.txt

run_slave:
	./bin/worker files/*.txt

run_view: 
	./bin/view /view

run_app_and_view:
	./bin/app ./files/*.txt | ./bin/view
