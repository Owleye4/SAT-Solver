# GNU Makefile for SAT solver (gcc)

CC := gcc
CFLAGS ?= -O2 -Wall -Wextra
LDFLAGS :=

SRC := parser.c solver.c main.c parser_opt.c
OBJ := $(SRC:.c=.o)
BIN := main

.PHONY: all clean run

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(BIN) cases/*.res

# Example: make run ARGS="cases/sat-20.cnf --timeout 5000 --model"
run: $(BIN)
	./$(BIN) $(ARGS)
