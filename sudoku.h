#ifndef SUDOKU_H
#define SUDOKU_H

#include "parser_opt.h"
#include "solver.h"
#include <time.h>
#include <string.h>

#define SUDOKU_SIZE 9
#define EMPTY_CELL 0

// Percent Sudoku structure
typedef struct {
    int grid[SUDOKU_SIZE][SUDOKU_SIZE];
    int given_count;  // Number of given cells
    int is_given[SUDOKU_SIZE][SUDOKU_SIZE];  // Mark which cells are initially given
} Sudoku;

// 数独管理函数
Sudoku* create_sudoku();
void free_sudoku(Sudoku* sudoku);

// 数独加载
int load_sudoku_from_string_format(Sudoku* sudoku, const char* line_str);

// SAT求解相关
OptCNF* sudoku_to_cnf(Sudoku* sudoku);
int solve_sudoku_with_sat(Sudoku* sudoku, const char* output_prefix);

// Windows GUI相关函数
#ifdef _WIN32
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

// GUI相关常量
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 700
#define GRID_SIZE 450
#define CELL_SIZE (GRID_SIZE / SUDOKU_SIZE)

// GUI函数声明
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void InitializeGUI();
void DrawSudoku(HDC hdc, Sudoku* sudoku);
void HandleCellClick(int x, int y, Sudoku* sudoku);
void HandleKeyboardInput(WPARAM wParam, Sudoku* sudoku);

#endif

#endif
