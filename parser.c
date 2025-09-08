#include "parser.h"
#include <string.h>
#include <ctype.h>

static void skip_line(FILE *fp) {
	int c;
	while ((c = fgetc(fp)) != EOF) {
		if (c == '\n') break;
	}
}

static int read_token(FILE *fp, char *buf, size_t buflen) {
	int c;
	// skip whitespace
	while ((c = fgetc(fp)) != EOF && isspace(c)) {}
	if (c == EOF) return 0;
	// read token
	size_t i = 0;
	buf[i++] = (char)c;
	while (i + 1 < buflen) {
		c = fgetc(fp);
		if (c == EOF || isspace(c)) break;
		buf[i++] = (char)c;
	}
	buf[i] = '\0';
	return 1;
}

static int parse_header(FILE *fp, int *num_vars, size_t *num_clauses) {
	char line[4096];
	while (fgets(line, sizeof(line), fp)) {
		char *p = line;
		while (*p && isspace((unsigned char)*p)) p++;
		if (*p == '\0') continue;
		if (*p == 'c' || *p == 'C') continue;
		if (*p == 'p' || *p == 'P') {
			int nv = 0; long nc = 0;
			// Accept standard form: p cnf <vars> <clauses>
			if (sscanf(p, "p cnf %d %ld", &nv, &nc) == 2 || sscanf(p, "P cnf %d %ld", &nv, &nc) == 2) {
				if (nv < 0 || nc < 0) return -1;
				*num_vars = nv;
				*num_clauses = (size_t)nc;
				return 0;
			}
			// Fallback tolerant parse: p <word> <vars> <clauses>
			char word[32];
			if (sscanf(p, "p %31s %d %ld", word, &nv, &nc) == 4) {
				if (strcmp(word, "cnf") != 0 && strcmp(word, "CNF") != 0) return -1;
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

int parse_cnf_file(const char *path, CNF *out) {
	if (!path || !out) return -1;
	memset(out, 0, sizeof(*out));
	FILE *fp = fopen(path, "r");
	if (!fp) return -1;
	int num_vars = 0;
	size_t num_clauses = 0;
	if (parse_header(fp, &num_vars, &num_clauses) != 0) {
		fclose(fp);
		return -1;
	}
	out->num_variables = num_vars;
	out->num_clauses = num_clauses;
	out->clauses = (Clause *)calloc(num_clauses, sizeof(Clause));
	if (!out->clauses) {
		fclose(fp);
		return -1;
	}

	// Parse clauses: a sequence of integers terminated by 0 for each clause
	char tok[64];
	size_t cls_idx = 0;
	int *tmp = NULL;
	size_t tmp_cap = 0, tmp_len = 0;
	while (cls_idx < num_clauses && read_token(fp, tok, sizeof(tok))) {
		if (tok[0] == 'c') { skip_line(fp); continue; }
		int lit = atoi(tok);
		if (lit == 0) {
			// end of clause, store it
			Clause *cl = &out->clauses[cls_idx++];
			if (tmp_len == 0) {
				cl->literals = NULL;
				cl->num_literals = 0;
			} else {
				cl->literals = (int *)malloc(tmp_len * sizeof(int));
				if (!cl->literals) { free(tmp); fclose(fp); return -1; }
				memcpy(cl->literals, tmp, tmp_len * sizeof(int));
				cl->num_literals = tmp_len;
			}
			tmp_len = 0;
			continue;
		}
		// grow tmp if needed
		if (tmp_len == tmp_cap) {
			size_t new_cap = tmp_cap ? tmp_cap * 2 : 8;
			int *new_arr = (int *)realloc(tmp, new_cap * sizeof(int));
			if (!new_arr) { free(tmp); fclose(fp); return -1; }
			tmp = new_arr;
			tmp_cap = new_cap;
		}
		tmp[tmp_len++] = lit;
	}
	free(tmp);
	// If fewer clauses than declared, adjust
	if (cls_idx != num_clauses) {
		out->num_clauses = cls_idx;
	}
	fclose(fp);
	return 0;
}

void free_cnf(CNF *cnf) {
	if (!cnf) return;
	if (cnf->clauses) {
		for (size_t i = 0; i < cnf->num_clauses; ++i) {
			free(cnf->clauses[i].literals);
		}
		free(cnf->clauses);
	}
	cnf->clauses = NULL;
	cnf->num_clauses = 0;
	cnf->num_variables = 0;
}

void print_cnf(const CNF *cnf, FILE *stream) {
	if (!cnf) return;
	FILE *out = stream ? stream : stdout;
	fprintf(out, "p cnf %d %zu\n", cnf->num_variables, cnf->num_clauses);
	for (size_t i = 0; i < cnf->num_clauses; ++i) {
		const Clause *cl = &cnf->clauses[i];
		for (size_t j = 0; j < cl->num_literals; ++j) {
			fprintf(out, "%d ", cl->literals[j]);
		}
		fprintf(out, "0\n");
	}
}


