#include "parser_opt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static int parse_header_line(FILE *fp, int *num_vars, size_t *num_clauses) {
	char line[4096];
	while (fgets(line, sizeof(line), fp)) {
		char *p = line;
		while (*p && isspace((unsigned char)*p)) p++;
		if (*p == '\0') continue;
		if (*p == 'c' || *p == 'C') continue;
		if (*p == 'p' || *p == 'P') {
			int nv = 0; long nc = 0;
			if (sscanf(p, "p cnf %d %ld", &nv, &nc) == 2 || sscanf(p, "P cnf %d %ld", &nv, &nc) == 2) {
				if (nv < 0 || nc < 0) return -1;
				*num_vars = nv;
				*num_clauses = (size_t)nc;
				return 0;
			}
			char tag[32];
			if (sscanf(p, "p %31s %d %ld", tag, &nv, &nc) == 4) {
				if (strcmp(tag, "cnf") != 0 && strcmp(tag, "CNF") != 0) return -1;
				if (nv < 0 || nc < 0) return -1;
				*num_vars = nv;
				*num_clauses = (size_t)nc;
				return 0;
			}
			return -1;
		}
	}
	return -1;
}

int parse_cnf_file_opt(const char *path, OptCNF *out) {
	if (!path || !out) return -1;
	memset(out, 0, sizeof(*out));
	FILE *fp = fopen(path, "r");
	if (!fp) return -1;
	int num_vars = 0;
	size_t num_clauses = 0;
	if (parse_header_line(fp, &num_vars, &num_clauses) != 0) { fclose(fp); return -1; }
	out->num_variables = num_vars;
	out->num_clauses = num_clauses;
	out->clauses = (OptClause *)calloc(num_clauses, sizeof(OptClause));
	if (!out->clauses) { fclose(fp); return -1; }

	// We'll push literals into a growing pool
	size_t pool_cap = 1024;
	out->literals_pool = (int *)malloc(pool_cap * sizeof(int));
	if (!out->literals_pool) { free(out->clauses); fclose(fp); return -1; }
	out->pool_len = 0;

	char token[64];
	int c;
	size_t clause_idx = 0;
	while (clause_idx < num_clauses) {
		// skip whitespace and comments
		c = fgetc(fp);
		if (c == EOF) break;
		if (isspace(c)) continue;
		if (c == 'c' || c == 'C') { // comment line
			while ((c = fgetc(fp)) != EOF && c != '\n') {}
			continue;
		}
		// start reading a token (possibly beginning of a clause)
		size_t len = 0;
		token[len++] = (char)c;
		while (len + 1 < sizeof(token)) {
			c = fgetc(fp);
			if (c == EOF || isspace(c)) break;
			token[len++] = (char)c;
		}
		token[len] = '\0';
		int lit = atoi(token);
		if (lit == 0) {
			// Empty token or stray zero: treat as clause separator; ignore
			continue;
		}
		// Begin a new clause: we already have first literal
		OptClause *cl = &out->clauses[clause_idx];
		cl->start_index = out->pool_len;
		cl->num_literals = 0;
		for (;;) {
			// push lit
			if (out->pool_len == pool_cap) {
				size_t new_cap = pool_cap * 2;
				int *new_pool = (int *)realloc(out->literals_pool, new_cap * sizeof(int));
				if (!new_pool) { fclose(fp); return -1; }
				out->literals_pool = new_pool;
				pool_cap = new_cap;
			}
			out->literals_pool[out->pool_len++] = lit;
			cl->num_literals++;
			// read next token until we hit 0
			// fast-scan numbers
			// skip spaces
			while ((c = fgetc(fp)) != EOF && isspace(c)) {}
			if (c == EOF) break;
			if (c == 'c' || c == 'C') { while ((c = fgetc(fp)) != EOF && c != '\n') {} continue; }
			// read token
			len = 0; token[len++] = (char)c;
			while (len + 1 < sizeof(token)) {
				c = fgetc(fp);
				if (c == EOF || isspace(c)) break;
				token[len++] = (char)c;
			}
			token[len] = '\0';
			lit = atoi(token);
			if (lit == 0) break; // end of clause
		}
		clause_idx++;
	}
	// Adjust actual number of clauses parsed
	if (clause_idx != num_clauses) out->num_clauses = clause_idx;
	fclose(fp);
	return 0;
}

void free_opt_cnf(OptCNF *cnf) {
	if (!cnf) return;
	free(cnf->clauses);
	free(cnf->literals_pool);
	cnf->clauses = NULL;
	cnf->literals_pool = NULL;
	cnf->num_clauses = 0;
	cnf->num_variables = 0;
	cnf->pool_len = 0;
}

int opt_cnf_to_cnf(const OptCNF *src, CNF *dst) {
	if (!src || !dst) return -1;
	memset(dst, 0, sizeof(*dst));
	dst->num_variables = src->num_variables;
	dst->num_clauses = src->num_clauses;
	dst->clauses = (Clause *)calloc(src->num_clauses, sizeof(Clause));
	if (!dst->clauses) return -1;
	for (size_t i = 0; i < src->num_clauses; ++i) {
		const OptClause *ocl = &src->clauses[i];
		Clause *cl = &dst->clauses[i];
		cl->num_literals = ocl->num_literals;
		if (ocl->num_literals == 0) {
			cl->literals = NULL;
			continue;
		}
		cl->literals = (int *)malloc(ocl->num_literals * sizeof(int));
		if (!cl->literals) return -1;
		memcpy(cl->literals, src->literals_pool + ocl->start_index, ocl->num_literals * sizeof(int));
	}
	return 0;
}


