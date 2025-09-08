#ifndef DISPLAY_H
#define DISPLAY_H

#ifdef _WIN32
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include "sudoku.h"

// Control IDs
#define ID_AUTO_SOLVE 1001
#define ID_CLEAR 1002
#define ID_PUZZLE_COMBO 1003
#define ID_LOAD_PUZZLE 1004

// Sudoku puzzle data
typedef struct {
    char puzzle_string[82];  // 81 characters + null terminator
    int index;
} SudokuPuzzle;

// Global variables
extern HWND g_hWnd;
extern Sudoku* g_sudoku;
extern int g_selected_row;
extern int g_selected_col;
extern HWND g_hAutoSolveBtn;
extern HWND g_hClearBtn;
extern HWND g_hPuzzleCombo;
extern HWND g_hLoadBtn;
extern char* g_input_filename;
extern SudokuPuzzle* g_puzzles;
extern int g_puzzle_count;
extern int g_current_puzzle_index;

// Function declarations
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void InitializeGUI();
void DrawSudoku(HDC hdc, Sudoku* sudoku);
void HandleCellClick(int x, int y, Sudoku* sudoku);
void HandleKeyboardInput(WPARAM wParam, Sudoku* sudoku);
int load_puzzles_from_file(const char* filename);
int load_selected_puzzle(int puzzle_index);
void parse_command_line();

#endif // _WIN32

#endif // DISPLAY_H
