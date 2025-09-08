#include "sudoku.h"

// Initialize Sudoku grid
static void init_sudoku_grid(Sudoku* sudoku) {
    sudoku->given_count = 0;
    for (int i = 0; i < SUDOKU_SIZE; i++) {
        for (int j = 0; j < SUDOKU_SIZE; j++) {
            sudoku->is_given[i][j] = 0;
            sudoku->grid[i][j] = EMPTY_CELL;
        }
    }
}

// Create Sudoku structure
Sudoku* create_sudoku() {
    Sudoku* sudoku = (Sudoku*)malloc(sizeof(Sudoku));
    if (!sudoku) return NULL;
    
    init_sudoku_grid(sudoku);
    return sudoku;
}

// Free Sudoku memory
void free_sudoku(Sudoku* sudoku) {
    if (sudoku) {
        free(sudoku);
    }
}

// Load Sudoku from string format (81 characters in a line)
int load_sudoku_from_string_format(Sudoku* sudoku, const char* line_str) {
    if (!sudoku || !line_str) return 0;
    
    init_sudoku_grid(sudoku);
    
    // Check if string has correct length
    size_t len = strlen(line_str);
    if (len < SUDOKU_SIZE * SUDOKU_SIZE) return 0;
    
    // Parse 81 characters
    for (int pos = 0; pos < SUDOKU_SIZE * SUDOKU_SIZE; pos++) {
        int row = pos / SUDOKU_SIZE;
        int col = pos % SUDOKU_SIZE;
        char ch = line_str[pos];
        
        int value = (ch == '.' || ch == '0') ? EMPTY_CELL : 
                   (ch >= '1' && ch <= '9') ? ch - '0' : -1;
        if (value == -1) continue; // Skip invalid characters
        
        sudoku->grid[row][col] = value;
        if (value != EMPTY_CELL) {
            sudoku->is_given[row][col] = 1;
            sudoku->given_count++;
        }
    }
    
    return 1;
}

// Convert Sudoku to CNF formula
OptCNF* sudoku_to_cnf(Sudoku* sudoku) {
    OptCNF* cnf = (OptCNF*)malloc(sizeof(OptCNF));
    if (!cnf) return NULL;
    
    // 数独有9*9*9 = 729个变量
    // 变量 x_{i,j,k} 表示位置(i,j)填入数字k
    // 编码：(i-1)*81 + (j-1)*9 + k
    cnf->num_variables = SUDOKU_SIZE * SUDOKU_SIZE * SUDOKU_SIZE;
    cnf->num_clauses = 0;
    cnf->clauses = NULL;
    cnf->literals_pool = NULL;
    cnf->pool_len = 0;
    
    // Pre-allocate memory pools
    size_t pool_capacity = 10000;
    size_t clauses_capacity = 2000;
    
    cnf->literals_pool = (int*)malloc(pool_capacity * sizeof(int));
    cnf->clauses = (OptClause*)malloc(clauses_capacity * sizeof(OptClause));
    if (!cnf->literals_pool || !cnf->clauses) {
        free(cnf->literals_pool);
        free(cnf->clauses);
        free(cnf);
        return NULL;
    }
    
    // 辅助函数：添加子句
    auto void add_clause(int* literals, int size) {
        if (cnf->num_clauses >= clauses_capacity) {
            clauses_capacity *= 2;
            cnf->clauses = realloc(cnf->clauses, clauses_capacity * sizeof(OptClause));
        }
        
        if (cnf->pool_len + size > pool_capacity) {
            pool_capacity = (cnf->pool_len + size) * 2;
            cnf->literals_pool = realloc(cnf->literals_pool, pool_capacity * sizeof(int));
        }
        
        OptClause* clause = &cnf->clauses[cnf->num_clauses];
        clause->start_index = cnf->pool_len;
        clause->num_literals = size;
        
        for (int i = 0; i < size; i++) {
            cnf->literals_pool[cnf->pool_len++] = literals[i];
        }
        cnf->num_clauses++;
    }
    
    // Helper macro for variable encoding
    #define VAR_ID(i, j, k) ((i-1)*81 + (j-1)*9 + k)
    
    int temp_literals[SUDOKU_SIZE];
    
    // 1. 每个格子必须至少有一个数字
    for (int i = 1; i <= SUDOKU_SIZE; i++) {
        for (int j = 1; j <= SUDOKU_SIZE; j++) {
            for (int k = 1; k <= SUDOKU_SIZE; k++) {
                temp_literals[k-1] = VAR_ID(i, j, k);
            }
            add_clause(temp_literals, SUDOKU_SIZE);
        }
    }
    
    // 2. 每个格子最多有一个数字
    for (int i = 1; i <= SUDOKU_SIZE; i++) {
        for (int j = 1; j <= SUDOKU_SIZE; j++) {
            for (int k1 = 1; k1 <= SUDOKU_SIZE; k1++) {
                for (int k2 = k1 + 1; k2 <= SUDOKU_SIZE; k2++) {
                    temp_literals[0] = -VAR_ID(i, j, k1);
                    temp_literals[1] = -VAR_ID(i, j, k2);
                    add_clause(temp_literals, 2);
                }
            }
        }
    }
    
    // 3. 每个数字在每行恰好出现一次
    for (int i = 1; i <= SUDOKU_SIZE; i++) {
        for (int k = 1; k <= SUDOKU_SIZE; k++) {
            // 至少一次
            for (int j = 1; j <= SUDOKU_SIZE; j++) {
                temp_literals[j-1] = (i-1)*81 + (j-1)*9 + k;
            }
            add_clause(temp_literals, SUDOKU_SIZE);
            
            // 最多一次
            for (int j1 = 1; j1 <= SUDOKU_SIZE; j1++) {
                for (int j2 = j1 + 1; j2 <= SUDOKU_SIZE; j2++) {
                    temp_literals[0] = -((i-1)*81 + (j1-1)*9 + k);
                    temp_literals[1] = -((i-1)*81 + (j2-1)*9 + k);
                    add_clause(temp_literals, 2);
                }
            }
        }
    }
    
    // 4. 每个数字在每列恰好出现一次
    for (int j = 1; j <= SUDOKU_SIZE; j++) {
        for (int k = 1; k <= SUDOKU_SIZE; k++) {
            // 至少一次
            for (int i = 1; i <= SUDOKU_SIZE; i++) {
                temp_literals[i-1] = (i-1)*81 + (j-1)*9 + k;
            }
            add_clause(temp_literals, SUDOKU_SIZE);
            
            // 最多一次
            for (int i1 = 1; i1 <= SUDOKU_SIZE; i1++) {
                for (int i2 = i1 + 1; i2 <= SUDOKU_SIZE; i2++) {
                    temp_literals[0] = -((i1-1)*81 + (j-1)*9 + k);
                    temp_literals[1] = -((i2-1)*81 + (j-1)*9 + k);
                    add_clause(temp_literals, 2);
                }
            }
        }
    }
    
    // 5. 每个数字在每个3x3方格恰好出现一次
    for (int box_row = 0; box_row < 3; box_row++) {
        for (int box_col = 0; box_col < 3; box_col++) {
            for (int k = 1; k <= SUDOKU_SIZE; k++) {
                // 至少一次
                int idx = 0;
                for (int i = 1; i <= 3; i++) {
                    for (int j = 1; j <= 3; j++) {
                        int row = box_row * 3 + i;
                        int col = box_col * 3 + j;
                        temp_literals[idx++] = (row-1)*81 + (col-1)*9 + k;
                    }
                }
                add_clause(temp_literals, 9);
                
                // 最多一次
                for (int pos1 = 0; pos1 < 9; pos1++) {
                    for (int pos2 = pos1 + 1; pos2 < 9; pos2++) {
                        int i1 = pos1 / 3 + 1, j1 = pos1 % 3 + 1;
                        int i2 = pos2 / 3 + 1, j2 = pos2 % 3 + 1;
                        int row1 = box_row * 3 + i1;
                        int col1 = box_col * 3 + j1;
                        int row2 = box_row * 3 + i2;
                        int col2 = box_col * 3 + j2;
                        
                        temp_literals[0] = -((row1-1)*81 + (col1-1)*9 + k);
                        temp_literals[1] = -((row2-1)*81 + (col2-1)*9 + k);
                        add_clause(temp_literals, 2);
                    }
                }
            }
        }
    }
    
    // 6. 百分号数独额外约束
    
    // 6.1 反对角线约束
    for (int k = 1; k <= SUDOKU_SIZE; k++) {
        // 至少一次
        for (int i = 0; i < SUDOKU_SIZE; i++) {
            int j = SUDOKU_SIZE - 1 - i;  // 反对角线位置
            temp_literals[i] = i*81 + j*9 + k;
        }
        add_clause(temp_literals, SUDOKU_SIZE);
        
        // 最多一次
        for (int i1 = 0; i1 < SUDOKU_SIZE; i1++) {
            for (int i2 = i1 + 1; i2 < SUDOKU_SIZE; i2++) {
                int j1 = SUDOKU_SIZE - 1 - i1;
                int j2 = SUDOKU_SIZE - 1 - i2;
                temp_literals[0] = -(i1*81 + j1*9 + k);
                temp_literals[1] = -(i2*81 + j2*9 + k);
                add_clause(temp_literals, 2);
            }
        }
    }
    
    // 6.2 上窗口约束：位置 (1,1) 到 (3,3)
    int upper_window_positions[9][2] = {{1,1},{1,2},{1,3},{2,1},{2,2},{2,3},{3,1},{3,2},{3,3}};
    for (int k = 1; k <= SUDOKU_SIZE; k++) {
        // 至少一次
        for (int p = 0; p < 9; p++) {
            int i = upper_window_positions[p][0];
            int j = upper_window_positions[p][1];
            temp_literals[p] = i*81 + j*9 + k;
        }
        add_clause(temp_literals, 9);
        
        // 最多一次
        for (int p1 = 0; p1 < 9; p1++) {
            for (int p2 = p1 + 1; p2 < 9; p2++) {
                int i1 = upper_window_positions[p1][0];
                int j1 = upper_window_positions[p1][1];
                int i2 = upper_window_positions[p2][0];
                int j2 = upper_window_positions[p2][1];
                temp_literals[0] = -(i1*81 + j1*9 + k);
                temp_literals[1] = -(i2*81 + j2*9 + k);
                add_clause(temp_literals, 2);
            }
        }
    }
    
    // 6.3 下窗口约束：位置 (5,5) 到 (7,7)
    int lower_window_positions[9][2] = {{5,5},{5,6},{5,7},{6,5},{6,6},{6,7},{7,5},{7,6},{7,7}};
    for (int k = 1; k <= SUDOKU_SIZE; k++) {
        // 至少一次
        for (int p = 0; p < 9; p++) {
            int i = lower_window_positions[p][0];
            int j = lower_window_positions[p][1];
            temp_literals[p] = i*81 + j*9 + k;
        }
        add_clause(temp_literals, 9);
        
        // 最多一次
        for (int p1 = 0; p1 < 9; p1++) {
            for (int p2 = p1 + 1; p2 < 9; p2++) {
                int i1 = lower_window_positions[p1][0];
                int j1 = lower_window_positions[p1][1];
                int i2 = lower_window_positions[p2][0];
                int j2 = lower_window_positions[p2][1];
                temp_literals[0] = -(i1*81 + j1*9 + k);
                temp_literals[1] = -(i2*81 + j2*9 + k);
                add_clause(temp_literals, 2);
            }
        }
    }
    
    // 7. 添加给定数字的约束
    for (int i = 0; i < SUDOKU_SIZE; i++) {
        for (int j = 0; j < SUDOKU_SIZE; j++) {
            if (sudoku->grid[i][j] != EMPTY_CELL) {
                int k = sudoku->grid[i][j];
                temp_literals[0] = i*81 + j*9 + k;
                add_clause(temp_literals, 1);
            }
        }
    }
    
    // CNF conversion completed
    
    return cnf;
}

// 使用SAT求解器解数独
int solve_sudoku_with_sat(Sudoku* sudoku, const char* output_prefix) {
    // Converting Sudoku to SAT problem
    // 转换为OptCNF
    OptCNF* opt_cnf = sudoku_to_cnf(sudoku);
    if (!opt_cnf) {
        return 0;
    }
    
    // 转换为标准CNF用于求解器
    CNF cnf;
    if (opt_cnf_to_cnf(opt_cnf, &cnf) != 0) {
        free_opt_cnf(opt_cnf);
        return 0;
    }
    
    // 保存CNF文件
    char cnf_filename[256];
    sprintf(cnf_filename, "%s.cnf", output_prefix);
    
    FILE* cnf_file = fopen(cnf_filename, "w");
    if (!cnf_file) {
        free_cnf(&cnf);
        free_opt_cnf(opt_cnf);
        return 0;
    }
    
    fprintf(cnf_file, "c Percent Sudoku SAT problem\n");
    fprintf(cnf_file, "p cnf %d %zu\n", cnf.num_variables, cnf.num_clauses);
    
    for (size_t i = 0; i < cnf.num_clauses; i++) {
        for (size_t j = 0; j < cnf.clauses[i].num_literals; j++) {
            fprintf(cnf_file, "%d ", cnf.clauses[i].literals[j]);
        }
        fprintf(cnf_file, "0\n");
    }
    
    fclose(cnf_file);
    // Solving Sudoku
    // 创建求解器并求解
    Assignment model;
    double solve_time_ms = 0.0;
    int result = dpll_solve(&cnf, &model, 30000, &solve_time_ms); // 30秒超时
    
    // 保存结果
    char res_filename[256];
    sprintf(res_filename, "%s.res", output_prefix);
    
    FILE* res_file = fopen(res_filename, "w");
    if (res_file) {
        int s_val = (result == 1) ? 1 : ((result == 0) ? 0 : -1);
        fprintf(res_file, "s %d\n", s_val);
        if (result == 1) {
            fprintf(res_file, "v ");
            for (int v = 1; v <= model.num_variables; ++v) {
                int lit = model.values[v] >= 0 ? v : -v;
                if (model.values[v] == 0) lit = v;
                fprintf(res_file, "%d ", lit);
            }
            fprintf(res_file, "\n");
        }
        fprintf(res_file, "t %.0f\n", solve_time_ms);
        fclose(res_file);
    }
    
    if (result == 1) {
        // Solution found
        
        // 验证解的正确性
        verify_model_satisfies(&cnf, &model);
        
        // 从解中恢复数独并填充到原数独中
        for (int i = 0; i < SUDOKU_SIZE; i++) {
            for (int j = 0; j < SUDOKU_SIZE; j++) {
                if (sudoku->grid[i][j] == EMPTY_CELL) {  // 只填充空格子
                    for (int k = 1; k <= SUDOKU_SIZE; k++) {
                        int var = i*81 + j*9 + k;
                        if (var <= model.num_variables && model.values[var] > 0) {
                            sudoku->grid[i][j] = k;
                            break;
                        }
                    }
                }
            }
        }
        
        // Solution filled into original sudoku grid
        free_assignment(&model);
    }
    
    free_cnf(&cnf);
    free_opt_cnf(opt_cnf);
    return (result == 1);
}
