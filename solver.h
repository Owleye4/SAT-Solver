// solver.h - DPLL SAT solver interface
#ifndef SAT_SOLVER_H
#define SAT_SOLVER_H

#include "parser.h"
#include <stddef.h>

typedef struct Assignment {
	// assignment for variables 1..num_variables
	// values: -1 = false, 0 = unassigned, 1 = true
	int *values;
	int num_variables;
} Assignment;

// Initialize assignment with all variables unassigned
int init_assignment(Assignment *a, int num_variables);
void free_assignment(Assignment *a);

// Run DPLL on the given CNF with an optional timeout in milliseconds.
// Returns: 1 = SAT, 0 = UNSAT, -1 = TIMEOUT/ABORT, -2 = ERROR.
// On SAT, fills 'model' with a satisfying assignment (size = num_variables).
// If timeout_ms <= 0, runs without a time limit.
// If out_time_ms is not NULL, writes measured solver time in milliseconds.
int dpll_solve(const CNF *cnf, Assignment *model, long timeout_ms, double *out_time_ms);

// Verify that the given assignment satisfies the CNF.
// Returns 1 if satisfied, 0 if any clause is unsatisfied, -1 on error.
int verify_model_satisfies(const CNF *cnf, const Assignment *model);

#endif // SAT_SOLVER_H


