// parser_opt.h - Optimized DIMACS CNF parser with pooled storage
#ifndef SAT_PARSER_OPT_H
#define SAT_PARSER_OPT_H

#include <stddef.h>
#include "parser.h"

typedef struct OptClause {
	size_t start_index;   // start index in literals_pool
	size_t num_literals;  // number of literals in this clause
} OptClause;

typedef struct OptCNF {
	int num_variables;
	size_t num_clauses;
	OptClause *clauses;   // array length = num_clauses
	int *literals_pool;   // contiguous pool of all clause literals
	size_t pool_len;      // number of used entries in pool
} OptCNF;

// Parse into optimized representation. Returns 0 on success.
int parse_cnf_file_opt(const char *path, OptCNF *out);

// Free optimized CNF memory.
void free_opt_cnf(OptCNF *cnf);

// Convert optimized CNF into standard CNF used by the solver.
// Returns 0 on success. Caller must later free_cnf on 'dst'.
int opt_cnf_to_cnf(const OptCNF *src, CNF *dst);

#endif // SAT_PARSER_OPT_H


