#include "solver.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

int init_assignment(Assignment *a, int num_variables) {
	if (!a || num_variables <= 0) return -1;
	a->values = (int *)malloc((size_t)(num_variables + 1) * sizeof(int));
	if (!a->values) return -1;
	a->num_variables = num_variables;
	for (int i = 0; i <= num_variables; ++i) a->values[i] = 0;
	return 0;
}

void free_assignment(Assignment *a) {
	if (!a) return;
	free(a->values);
	a->values = NULL;
	a->num_variables = 0;
}

static inline int lit_var(int lit) { return lit > 0 ? lit : -lit; }
static inline int lit_sign(int lit) { return lit > 0 ? 1 : -1; }

// Evaluate a literal under assignment: 1 true, -1 false, 0 unassigned
static int eval_lit(const Assignment *as, int lit) {
	int v = lit_var(lit);
	int val = as->values[v];
	if (val == 0) return 0;
	return (val == lit_sign(lit)) ? 1 : -1;
}

// Unit propagation: returns 1 if consistent, 0 if conflict. Also detects empty formula success.
// Uses a simple loop scanning all clauses until no changes.
static int unit_propagate(const CNF *cnf, Assignment *as) {
	int changed = 1;
	while (changed) {
		changed = 0;
		for (size_t i = 0; i < cnf->num_clauses; ++i) {
			const Clause *cl = &cnf->clauses[i];
			int num_unassigned = 0;
			int last_unassigned_lit = 0;
			int clause_satisfied = 0;
			for (size_t j = 0; j < cl->num_literals; ++j) {
				int lit = cl->literals[j];
				int val = eval_lit(as, lit);
				if (val == 1) { clause_satisfied = 1; break; }
				if (val == 0) { num_unassigned++; last_unassigned_lit = lit; }
			}
			if (clause_satisfied) continue;
			if (num_unassigned == 0) {
				// Clause falsified under current partial assignment
				return 0;
			}
			if (num_unassigned == 1) {
				int var = lit_var(last_unassigned_lit);
				int sign = lit_sign(last_unassigned_lit);
				if (as->values[var] == 0) {
					as->values[var] = sign;
					changed = 1;
				} else if (as->values[var] != sign) {
					return 0;
				}
			}
		}
	}
	return 1;
}

static int choose_unassigned_variable(const CNF *cnf, const Assignment *as) {
	for (int v = 1; v <= cnf->num_variables; ++v) {
		if (as->values[v] == 0) return v;
	}
	return -1;
}

static int all_clauses_satisfied(const CNF *cnf, const Assignment *as) {
	for (size_t i = 0; i < cnf->num_clauses; ++i) {
		const Clause *cl = &cnf->clauses[i];
		int sat = 0;
		for (size_t j = 0; j < cl->num_literals; ++j) {
			if (eval_lit(as, cl->literals[j]) == 1) { sat = 1; break; }
		}
		if (!sat) return 0;
	}
	return 1;
}

typedef struct SolverCtx {
	const CNF *cnf;
	Assignment *assignment;
	clock_t start_clock;
	long timeout_ms; // <= 0 means no timeout
} SolverCtx;

static int timed_out(const SolverCtx *ctx) {
	if (ctx->timeout_ms <= 0) return 0;
	clock_t now = clock();
	double elapsed_ms = (double)(now - ctx->start_clock) * 1000.0 / (double)CLOCKS_PER_SEC;
	return elapsed_ms > (double)ctx->timeout_ms;
}

static int dpll_recursive_ctx(SolverCtx *ctx) {
	if (timed_out(ctx)) return -1;
	if (!unit_propagate(ctx->cnf, ctx->assignment)) return 0;
	if (all_clauses_satisfied(ctx->cnf, ctx->assignment)) return 1;
	int var = choose_unassigned_variable(ctx->cnf, ctx->assignment);
	if (var == -1) return 0;

	// Snapshot assignment for proper backtracking
	int num = ctx->assignment->num_variables;
	int *backup = (int *)malloc((size_t)(num + 1) * sizeof(int));
	if (!backup) return -1;
	memcpy(backup, ctx->assignment->values, (size_t)(num + 1) * sizeof(int));

	// Branch var = True
	ctx->assignment->values[var] = 1;
	int r = dpll_recursive_ctx(ctx);
	if (r != 0) { free(backup); return (r == -1) ? -1 : 1; }

	// Restore and try var = False
	memcpy(ctx->assignment->values, backup, (size_t)(num + 1) * sizeof(int));
	ctx->assignment->values[var] = -1;
	r = dpll_recursive_ctx(ctx);
	if (r != 0) { free(backup); return (r == -1) ? -1 : 1; }

	// Restore and report UNSAT for this branch point
	memcpy(ctx->assignment->values, backup, (size_t)(num + 1) * sizeof(int));
	free(backup);
	return 0;
}

int dpll_solve(const CNF *cnf, Assignment *model, long timeout_ms, double *out_time_ms) {
	if (!cnf || !model) return -2;
	if (init_assignment(model, cnf->num_variables) != 0) return -2;
	SolverCtx ctx;
	ctx.cnf = cnf;
	ctx.assignment = model;
	ctx.timeout_ms = timeout_ms;
	ctx.start_clock = clock();
	int r = dpll_recursive_ctx(&ctx);
	clock_t end_clock = clock();
	double ms = (double)(end_clock - ctx.start_clock) * 1000.0 / (double)CLOCKS_PER_SEC;
	if (out_time_ms) *out_time_ms = ms;
	if (r == 1) return 1;
	if (r == -1) { free_assignment(model); return -1; }
	free_assignment(model);
	return 0;
}

int verify_model_satisfies(const CNF *cnf, const Assignment *model) {
	if (!cnf || !model || !model->values) return -1;
	for (size_t i = 0; i < cnf->num_clauses; ++i) {
		const Clause *cl = &cnf->clauses[i];
		int sat = 0;
		for (size_t j = 0; j < cl->num_literals; ++j) {
			int lit = cl->literals[j];
			int var = lit > 0 ? lit : -lit;
			if (var < 1 || var > model->num_variables) return -1;
			int val = model->values[var];
			if ((lit > 0 && val == 1) || (lit < 0 && val == -1)) { sat = 1; break; }
		}
		if (!sat) return 0;
	}
	return 1;
}


