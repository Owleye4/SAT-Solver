// parser.h - DIMACS CNF parser and data structures
#ifndef SAT_PARSER_H
#define SAT_PARSER_H

#include <stdio.h>
#include <stdlib.h>

// Literal encoding: integers where sign indicates polarity and absolute
// value indicates variable id in range [1, num_variables].

typedef struct Clause {
	int *literals;      // dynamic array of literals (e.g., -3, 7, ...)
	size_t num_literals;
} Clause;

typedef struct CNF {
	int num_variables;  // number of variables declared in DIMACS header
	size_t num_clauses; // number of clauses declared in DIMACS header
	Clause *clauses;    // dynamic array of clauses
} CNF;

// Parse a DIMACS CNF file at path into the provided CNF struct.
// Returns 0 on success, non-zero on failure.
int parse_cnf_file(const char *path, CNF *out);

// Free memory associated with a CNF structure.
void free_cnf(CNF *cnf);

// Print the CNF in a readable format for validation.
// If stream is NULL, stdout is used.
void print_cnf(const CNF *cnf, FILE *stream);

#endif // SAT_PARSER_H


