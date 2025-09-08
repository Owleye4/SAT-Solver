# Simplified Makefile for SAT Solver and Sudoku GUI

CC := gcc
CFLAGS := -O2 -Wall -Wextra
WIN32_FLAGS := -D_WIN32 -mwindows -lcomctl32

# Targets
SAT_BIN := sat_solver
GUI_BIN := display

# Source files
SAT_SOURCES := parser.c solver.c sat_solver.c parser_opt.c
GUI_SOURCES := sudoku.c display.c
SHARED_SOURCES := parser.c solver.c parser_opt.c

.PHONY: all clean

# Default target
all: $(SAT_BIN) $(GUI_BIN)

# SAT Solver
$(SAT_BIN): $(SAT_SOURCES:.c=.o)
	$(CC) $(CFLAGS) -o $@ $^

# GUI (Windows only)
$(GUI_BIN): $(GUI_SOURCES:.c=.o) $(SHARED_SOURCES:.c=.o)
	$(CC) $(CFLAGS) $(WIN32_FLAGS) -o $@ $^

# GUI-specific compilation
display.o: display.c
	$(CC) $(CFLAGS) -D_WIN32 -c $< -o $@

# Generic compilation rule
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	rm -f *.o *.exe sudoku/*.cnf sudoku/*.res cases/*.res
