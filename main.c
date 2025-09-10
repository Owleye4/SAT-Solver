#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "parser.h"
#include "solver.h"
#include "parser_opt.h"
#include "sudoku.h"

#ifdef _WIN32
#include "display.h"
#endif

// Function prototypes
void run_sat_mode();
void run_sudoku_mode();
void print_usage(const char *prog);
void print_mode_menu();

// SAT solver mode - similar to sat_solver.c but integrated
void run_sat_mode() {
    printf("\n=== SAT Solver Mode ===\n");
    printf("Enter the path to a CNF file: ");
    
    char path[1024];
    if (fgets(path, sizeof(path), stdin) == NULL) {
        printf("Error reading input.\n");
        return;
    }
    
    // Remove newline character
    path[strcspn(path, "\n")] = 0;
    
    if (strlen(path) == 0) {
        printf("No file path provided.\n");
        return;
    }
    
    printf("Enter options (--print, --model, --check, --timeout MS) or press Enter for none: ");
    char options[1024];
    if (fgets(options, sizeof(options), stdin) == NULL) {
        printf("Error reading input.\n");
        return;
    }
    
    // Remove newline character
    options[strcspn(options, "\n")] = 0;
    
    // Parse options
    int do_print = 0;
    int do_model = 0;
    int do_check = 0;
    long timeout_ms = 0;
    
    char *token = strtok(options, " ");
    while (token != NULL) {
        if (strcmp(token, "--print") == 0) do_print = 1;
        else if (strcmp(token, "--model") == 0) do_model = 1;
        else if (strcmp(token, "--check") == 0) do_check = 1;
        else if (strcmp(token, "--timeout") == 0) {
            token = strtok(NULL, " ");
            if (token != NULL) timeout_ms = atol(token);
        }
        token = strtok(NULL, " ");
    }
    
    // Parse CNF file
    CNF cnf;
    clock_t p0 = clock();
    if (parse_cnf_file(path, &cnf) != 0) {
        printf("Failed to parse CNF file: %s\n", path);
        return;
    }
    clock_t p1 = clock();
    double t_parse_ms = (double)(p1 - p0) * 1000.0 / (double)CLOCKS_PER_SEC;
    
    if (do_print) {
        print_cnf(&cnf, stdout);
    }
    
    // Optimized parser timing
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
    
    // Solve
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
        printf("Failed to open result file: %s\n", outpath);
        free_cnf(&cnf);
        if (res == 1) free_assignment(&model);
        return;
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
            if (model.values[v] == 0) lit = v;
            fprintf(rf, "%d ", lit);
        }
        fprintf(rf, "\n");
    }
    fprintf(rf, "t %.0f\n", ms);
    fclose(rf);
    
    // Console output
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
    
    // Print parser timing comparison
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
}

// Sudoku mode - launches GUI on Windows, console mode on other platforms
void run_sudoku_mode() {
    printf("\n=== Sudoku Mode ===\n");
    
#ifdef _WIN32
    printf("Enter the path to a Sudoku puzzle file: ");
    char filename[1024];
    if (fgets(filename, sizeof(filename), stdin) == NULL) {
        printf("Error reading input.\n");
        return;
    }
    
    // Remove newline character
    filename[strcspn(filename, "\n")] = 0;
    
    if (strlen(filename) == 0) {
        printf("No file path provided.\n");
        return;
    }
    
    printf("Launching Sudoku GUI...\n");
    
    // Set the input filename for the GUI
    g_input_filename = malloc(strlen(filename) + 1);
    if (g_input_filename) {
        strcpy(g_input_filename, filename);
    }
    
    // Initialize and run GUI
    InitializeGUI();
    
    if (!g_hWnd) {
        printf("Failed to create GUI window.\n");
        if (g_input_filename) {
            free(g_input_filename);
            g_input_filename = NULL;
        }
        return;
    }
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    if (g_input_filename) {
        free(g_input_filename);
        g_input_filename = NULL;
    }
#else
    printf("Sudoku GUI is only available on Windows.\n");
    printf("You can use the separate 'display' executable for GUI functionality.\n");
#endif
}

void print_usage(const char *prog) {
    printf("Usage: %s [mode]\n", prog);
    printf("Modes:\n");
    printf("  SAT     - Run SAT solver mode\n");
    printf("  %%-Sudoku - Run Sudoku mode (Windows GUI)\n");
    printf("  (no args) - Interactive mode selection\n");
}

void print_mode_menu() {
    printf("\n=== SAT Solver & Sudoku Integration ===\n");
    printf("Select mode:\n");
    printf("1. SAT Solver Mode\n");
    printf("2. Sudoku Mode (Windows GUI)\n");
    printf("3. Exit\n");
    printf("Enter choice (1-3): ");
}

int main(int argc, char **argv) {
    // Check for command line arguments
    if (argc > 1) {
        if (strcmp(argv[1], "SAT") == 0) {
            run_sat_mode();
            return 0;
        } else if (strcmp(argv[1], "%-Sudoku") == 0) {
            run_sudoku_mode();
            return 0;
        } else if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            printf("Unknown mode: %s\n", argv[1]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    // Interactive mode selection
    int choice;
    while (1) {
        print_mode_menu();
        
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            // Clear input buffer
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            continue;
        }
        
        // Clear input buffer
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        
        switch (choice) {
            case 1:
                run_sat_mode();
                break;
            case 2:
                run_sudoku_mode();
                break;
            case 3:
                printf("Goodbye!\n");
                return 0;
            default:
                printf("Invalid choice. Please enter 1, 2, or 3.\n");
                break;
        }
        
        printf("\nPress Enter to continue...");
        getchar();
    }
    
    return 0;
}
