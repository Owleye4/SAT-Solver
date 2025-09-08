#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "parser.h"
#include "solver.h"
#include "parser_opt.h"

static void usage(const char *prog) {
	fprintf(stderr, "Usage: %s <input.cnf> [--print] [--model] [--timeout MS] [--check]\n", prog);
}

int main(int argc, char **argv) {
	if (argc < 2) { usage(argv[0]); return 1; }
	const char *path = argv[1];
	int do_print = 0;
	int do_model = 0;
	int do_check = 0;
	long timeout_ms = 0;
	for (int i = 2; i < argc; ++i) {
		if (strcmp(argv[i], "--print") == 0) do_print = 1;
		else if (strcmp(argv[i], "--model") == 0) do_model = 1;
		else if (strcmp(argv[i], "--check") == 0) do_check = 1;
		else if (strcmp(argv[i], "--timeout") == 0 && i + 1 < argc) { timeout_ms = atol(argv[++i]); }
		else { usage(argv[0]); return 1; }
	}

	CNF cnf;
	clock_t p0 = clock();
	if (parse_cnf_file(path, &cnf) != 0) {
		fprintf(stderr, "Failed to parse CNF file: %s\n", path);
		return 1;
	}
	clock_t p1 = clock();
	double t_parse_ms = (double)(p1 - p0) * 1000.0 / (double)CLOCKS_PER_SEC;
	if (do_print) {
		print_cnf(&cnf, stdout);
	}

	// Optimized parser timing: parse OptCNF and convert to CNF
	OptCNF ocnf;
	clock_t q0 = clock();
	int opt_ok = parse_cnf_file_opt(path, &ocnf);
	CNF cnf_opt;
	int conv_ok = 0;
	if (opt_ok == 0) {
		conv_ok = (opt_cnf_to_cnf(&ocnf, &cnf_opt) == 0);
		free_opt_cnf(&ocnf);
	}
	clock_t q1 = clock();
	double t_parse_opt_ms = (double)(q1 - q0) * 1000.0 / (double)CLOCKS_PER_SEC;

	Assignment model;
	double ms = 0.0;
	int res = dpll_solve(&cnf, &model, timeout_ms, &ms);

	// Prepare .res file path
	char outpath[4096];
	const char *dot = strrchr(path, '.');
	if (!dot) dot = path + strlen(path);
	size_t base_len = (size_t)(dot - path);
	if (base_len >= sizeof(outpath) - 5) base_len = sizeof(outpath) - 5;
	memcpy(outpath, path, base_len);
	memcpy(outpath + base_len, ".res", 5);

	FILE *rf = fopen(outpath, "w");
	if (!rf) {
		fprintf(stderr, "Failed to open result file: %s\n", outpath);
		free_cnf(&cnf);
		if (res == 1) free_assignment(&model);
		return 3;
	}

	int s_val = 0;
	if (res == 1) s_val = 1;
	else if (res == 0) s_val = 0;
	else if (res == -1) s_val = -1;
	else s_val = -1;
	fprintf(rf, "s %d\n", s_val);
	if (res == 1) {
		fprintf(rf, "v ");
		for (int v = 1; v <= model.num_variables; ++v) {
			int lit = model.values[v] >= 0 ? v : -v;
			if (model.values[v] == 0) lit = v; // default unassigned to positive
			fprintf(rf, "%d ", lit);
		}
		fprintf(rf, "\n");
	}
	fprintf(rf, "t %.0f\n", ms);
	fclose(rf);

	// Optional console output
	if (res == 1) {
		printf("SAT (%.0f ms) -> %s\n", ms, outpath);
		if (do_model) {
			for (int v = 1; v <= model.num_variables; ++v) {
				int lit = model.values[v] >= 0 ? v : -v;
				if (model.values[v] == 0) lit = v;
				printf("%d ", lit);
			}
			printf("0\n");
		}
		if (do_check) {
			int ok = verify_model_satisfies(&cnf, &model);
			printf("check: %s\n", ok == 1 ? "OK" : (ok == 0 ? "FAIL" : "ERROR"));
		}
		free_assignment(&model);
	} else if (res == 0) {
		printf("UNSAT (%.0f ms) -> %s\n", ms, outpath);
	} else if (res == -1) {
		printf("TIMEOUT (%.0f ms) -> %s\n", ms, outpath);
	} else {
		printf("ERROR -> %s\n", outpath);
	}

	// Print parser timing comparison and optimization rate
	if (conv_ok) {
		printf("parse_ms=%.0f parse_opt_ms=%.0f", t_parse_ms, t_parse_opt_ms);
		if (t_parse_ms > 0.0) {
			double rate = (t_parse_ms - t_parse_opt_ms) / t_parse_ms * 100.0;
			printf(" optimize=%.2f%%\n", rate);
		} else {
			printf("\n");
		}
		free_cnf(&cnf_opt);
	} else {
		printf("parse_ms=%.0f\n", t_parse_ms);
	}
	free_cnf(&cnf);
	return 0;
}


