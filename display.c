#ifdef _WIN32
#include "display.h"

// Global variable definitions
HWND g_hWnd = NULL;
Sudoku* g_sudoku = NULL;
int g_selected_row = -1;
int g_selected_col = -1;
HWND g_hAutoSolveBtn = NULL;
HWND g_hClearBtn = NULL;
HWND g_hPuzzleCombo = NULL;
HWND g_hLoadBtn = NULL;
char* g_input_filename = NULL;
SudokuPuzzle* g_puzzles = NULL;
int g_puzzle_count = 0;
int g_current_puzzle_index = 0;

// Helper function to check if a line is a valid puzzle
static int is_valid_puzzle_line(const char* line) {
    // Skip comment lines and empty lines
    if ((line[0] == '/' && line[1] == '/') || line[0] == '\n' || line[0] == '\r') {
        return 0;
    }
    
    // Check if line has 81 characters (puzzle format)
    return (strlen(line) >= 81);
}

// Function to load puzzles from file
int load_puzzles_from_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        return 0;
    }
    
    char line[256];
    g_puzzle_count = 0;
    
    // Count puzzles first
    while (fgets(line, sizeof(line), file)) {
        if (is_valid_puzzle_line(line)) {
            g_puzzle_count++;
        }
    }
    
    if (g_puzzle_count == 0) {
        fclose(file);
        return 0;
    }
    
    // Allocate memory for puzzles
    g_puzzles = malloc(g_puzzle_count * sizeof(SudokuPuzzle));
    if (!g_puzzles) {
        fclose(file);
        return 0;
    }
    
    // Read puzzles
    rewind(file);
    int puzzle_index = 0;
    
    while (fgets(line, sizeof(line), file) && puzzle_index < g_puzzle_count) {
        if (is_valid_puzzle_line(line)) {
            // Copy first 81 characters
            strncpy(g_puzzles[puzzle_index].puzzle_string, line, 81);
            g_puzzles[puzzle_index].puzzle_string[81] = '\0';
            g_puzzles[puzzle_index].index = puzzle_index + 1;
            puzzle_index++;
        }
    }
    
    fclose(file);
    return 1;
}

// Function to load selected puzzle into sudoku
int load_selected_puzzle(int puzzle_index) {
    if (!g_puzzles || puzzle_index < 0 || puzzle_index >= g_puzzle_count) {
        return 0;
    }
    
    if (!g_sudoku) {
        g_sudoku = create_sudoku();
        if (!g_sudoku) return 0;
    }
    
    return load_sudoku_from_string_format(g_sudoku, g_puzzles[puzzle_index].puzzle_string);
}

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            // Create puzzle selection controls (moved to top)
            CreateWindow(
                "STATIC", "Select Puzzle:",
                WS_VISIBLE | WS_CHILD,
                50, 80, 100, 20,
                hwnd, NULL, GetModuleHandle(NULL), NULL
            );
            
            g_hPuzzleCombo = CreateWindow(
                "COMBOBOX", "",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
                160, 78, 200, 200,
                hwnd, (HMENU)ID_PUZZLE_COMBO, GetModuleHandle(NULL), NULL
            );
            
            g_hLoadBtn = CreateWindow(
                "BUTTON", "Load Puzzle",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                370, 78, 100, 25,
                hwnd, (HMENU)ID_LOAD_PUZZLE, GetModuleHandle(NULL), NULL
            );
            
            // Create buttons (moved down)
            g_hAutoSolveBtn = CreateWindow(
                "BUTTON", "Auto Solve",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                550, 250, 120, 40,
                hwnd, (HMENU)ID_AUTO_SOLVE, GetModuleHandle(NULL), NULL
            );
            
            g_hClearBtn = CreateWindow(
                "BUTTON", "Clear",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                680, 250, 80, 40,
                hwnd, (HMENU)ID_CLEAR, GetModuleHandle(NULL), NULL
            );
            
            // Initialize empty sudoku (will be loaded from file)
            g_sudoku = create_sudoku();
            
            // Load puzzles from command line specified file
            if (g_input_filename && load_puzzles_from_file(g_input_filename)) {
                // Populate combo box
                for (int i = 0; i < g_puzzle_count; i++) {
                    char item_text[50];
                    sprintf(item_text, "Puzzle %d", i + 1);
                    SendMessage(g_hPuzzleCombo, CB_ADDSTRING, 0, (LPARAM)item_text);
                }
                SendMessage(g_hPuzzleCombo, CB_SETCURSEL, 0, 0); // Select first item
            } else {
                char error_msg[256];
                if (g_input_filename) {
                    sprintf(error_msg, "Cannot load file: %s", g_input_filename);
                } else {
                    sprintf(error_msg, "No input file specified! Usage: sudoku_gui.exe <filename>.txt");
                }
                MessageBoxA(hwnd, error_msg, "Error", MB_OK | MB_ICONERROR);
            }
            
            return 0;
        }
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            // Draw sudoku grid
            DrawSudoku(hdc, g_sudoku);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_LBUTTONDOWN: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            HandleCellClick(x, y, g_sudoku);
            // Set focus to window to receive keyboard input
            SetFocus(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;
        }
        
        case WM_KEYDOWN: {
            HandleKeyboardInput(wParam, g_sudoku);
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;
        }
        
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case ID_LOAD_PUZZLE: {
                    int selected_index = SendMessage(g_hPuzzleCombo, CB_GETCURSEL, 0, 0);
                    if (selected_index != CB_ERR && selected_index < g_puzzle_count) {
                        if (load_selected_puzzle(selected_index)) {
                            g_current_puzzle_index = selected_index;
                            g_selected_row = g_selected_col = -1;
                            InvalidateRect(hwnd, NULL, TRUE);
                        } else {
                            MessageBoxA(hwnd, "Failed to load selected puzzle!", "Error", MB_OK | MB_ICONERROR);
                        }
                    }
                    break;
                }
                
                case ID_AUTO_SOLVE: {
                    if (g_sudoku) {
                        // Refresh interface to show "solving..."
                        InvalidateRect(hwnd, NULL, TRUE);
                        UpdateWindow(hwnd);
                        
                        // Call SAT solver
                        int result = solve_sudoku_with_sat(g_sudoku, "sudoku/sudoku");
                        
                        if (result) {
                            MessageBoxA(hwnd, "Solution found! CNF and result files generated.", "Success", MB_OK);
                        } else {
                            MessageBoxA(hwnd, "No solution found or solving timeout", "Failed", MB_OK);
                        }
                        
                        InvalidateRect(hwnd, NULL, TRUE);
                    }
                    break;
                }
                
                case ID_CLEAR: {
                    if (g_sudoku) {
                        // Clear all user-filled cells (keep only given cells)
                        for (int i = 0; i < SUDOKU_SIZE; i++) {
                            for (int j = 0; j < SUDOKU_SIZE; j++) {
                                if (!g_sudoku->is_given[i][j]) {
                                    g_sudoku->grid[i][j] = EMPTY_CELL;
                                }
                            }
                        }
                        g_selected_row = g_selected_col = -1;
                        InvalidateRect(hwnd, NULL, TRUE);
                    }
                    break;
                }
            }
            return 0;
        }
        
        case WM_DESTROY:
            if (g_sudoku) {
                free_sudoku(g_sudoku);
                g_sudoku = NULL;
            }
            if (g_puzzles) {
                free(g_puzzles);
                g_puzzles = NULL;
            }
            if (g_input_filename) {
                free(g_input_filename);
                g_input_filename = NULL;
            }
            PostQuitMessage(0);
            return 0;
            
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

// Draw sudoku grid
void DrawSudoku(HDC hdc, Sudoku* sudoku) {
    if (!sudoku) return;
    
    // Set background color
    RECT clientRect;
    GetClientRect(g_hWnd, &clientRect);
    HBRUSH hBrush = CreateSolidBrush(RGB(240, 240, 240));
    FillRect(hdc, &clientRect, hBrush);
    DeleteObject(hBrush);
    
    // Calculate grid position (moved down to accommodate puzzle selector)
    int startX = 50;
    int startY = 120;  // Moved down from 80 to 120
    
    // First draw light blue background for constraint areas
    HBRUSH hLightBlueBrush = CreateSolidBrush(RGB(173, 216, 230)); // Light blue
    
    // Anti-diagonal area
    for (int i = 0; i < SUDOKU_SIZE; i++) {
        int j = SUDOKU_SIZE - 1 - i;
        int cellX = startX + j * CELL_SIZE;
        int cellY = startY + i * CELL_SIZE;
        RECT rect = {cellX + 1, cellY + 1, cellX + CELL_SIZE - 1, cellY + CELL_SIZE - 1};
        FillRect(hdc, &rect, hLightBlueBrush);
    }
    
    // Upper window (1,1) to (3,3)
    RECT upperRect = {startX + 1 * CELL_SIZE + 1, startY + 1 * CELL_SIZE + 1,
                      startX + 4 * CELL_SIZE - 1, startY + 4 * CELL_SIZE - 1};
    FillRect(hdc, &upperRect, hLightBlueBrush);
    
    // Lower window (5,5) to (7,7)
    RECT lowerRect = {startX + 5 * CELL_SIZE + 1, startY + 5 * CELL_SIZE + 1,
                      startX + 8 * CELL_SIZE - 1, startY + 8 * CELL_SIZE - 1};
    FillRect(hdc, &lowerRect, hLightBlueBrush);
    
    DeleteObject(hLightBlueBrush);
    
    // Then draw grid lines
    HPEN hPenThin = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
    HPEN hPenThick = CreatePen(PS_SOLID, 3, RGB(0, 0, 0));
    
    // Thin lines (cell borders)
    SelectObject(hdc, hPenThin);
    for (int i = 0; i <= SUDOKU_SIZE; i++) {
        // Vertical lines
        MoveToEx(hdc, startX + i * CELL_SIZE, startY, NULL);
        LineTo(hdc, startX + i * CELL_SIZE, startY + GRID_SIZE);
        
        // Horizontal lines
        MoveToEx(hdc, startX, startY + i * CELL_SIZE, NULL);
        LineTo(hdc, startX + GRID_SIZE, startY + i * CELL_SIZE);
    }
    
    // Thick lines (3x3 box borders)
    SelectObject(hdc, hPenThick);
    for (int i = 0; i <= 3; i++) {
        // Vertical lines
        MoveToEx(hdc, startX + i * 3 * CELL_SIZE, startY, NULL);
        LineTo(hdc, startX + i * 3 * CELL_SIZE, startY + GRID_SIZE);
        
        // Horizontal lines
        MoveToEx(hdc, startX, startY + i * 3 * CELL_SIZE, NULL);
        LineTo(hdc, startX + GRID_SIZE, startY + i * 3 * CELL_SIZE);
    }
    
    // Draw selected cell
    if (g_selected_row >= 0 && g_selected_col >= 0) {
        HPEN hPenSelect = CreatePen(PS_SOLID, 3, RGB(255, 255, 0));
        SelectObject(hdc, hPenSelect);
        int cellX = startX + g_selected_col * CELL_SIZE;
        int cellY = startY + g_selected_row * CELL_SIZE;
        Rectangle(hdc, cellX, cellY, cellX + CELL_SIZE, cellY + CELL_SIZE);
        DeleteObject(hPenSelect);
    }
    
    // Draw numbers
    SetBkMode(hdc, TRANSPARENT);
    HFONT hFont = CreateFont(32, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial");
    SelectObject(hdc, hFont);

    for (int i = 0; i < SUDOKU_SIZE; i++) {
        for (int j = 0; j < SUDOKU_SIZE; j++) {
            if (sudoku->grid[i][j] != EMPTY_CELL) {
                char numStr[2];
                sprintf(numStr, "%d", sudoku->grid[i][j]);

                int cellX = startX + j * CELL_SIZE;
                int cellY = startY + i * CELL_SIZE;

                // Set different colors based on whether it's an initial cell
                if (sudoku->is_given[i][j]) {
                    SetTextColor(hdc, RGB(0, 0, 0));  // Given numbers in black
                } else {
                    SetTextColor(hdc, RGB(0, 50, 150));  // Solved numbers in blue
                }
                TextOut(hdc, cellX + CELL_SIZE/2 - 8, cellY + CELL_SIZE/2 - 16, numStr, 1);
            }
        }
    }

    // Draw title and description
    HFONT hTitleFont = CreateFont(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial");
    SelectObject(hdc, hTitleFont);
    SetTextColor(hdc, RGB(0, 0, 0));
    TextOut(hdc, 50, 20, "Percent Sudoku", 13);

    // Clean up resources
    DeleteObject(hPenThin);
    DeleteObject(hPenThick);
    DeleteObject(hFont);
    DeleteObject(hTitleFont);
}

// Handle cell click
void HandleCellClick(int x, int y, Sudoku* sudoku) {
    (void)sudoku; // Avoid unused parameter warning
    int startX = 50;
    int startY = 120;  // Updated to match new grid position
    
    if (x >= startX && x < startX + GRID_SIZE && 
        y >= startY && y < startY + GRID_SIZE) {
        
        int col = (x - startX) / CELL_SIZE;
        int row = (y - startY) / CELL_SIZE;
        
        if (row >= 0 && row < SUDOKU_SIZE && col >= 0 && col < SUDOKU_SIZE) {
            // Only select cell, don't show dialog
            g_selected_row = row;
            g_selected_col = col;
        }
    }
}

// Handle keyboard input
void HandleKeyboardInput(WPARAM wParam, Sudoku* sudoku) {
    if (!sudoku || g_selected_row < 0 || g_selected_col < 0) {
        return; // No cell selected
    }
    
    // Check if it's an initial cell, if so don't allow modification
    if (sudoku->is_given[g_selected_row][g_selected_col]) {
        return; // Initial cells cannot be modified
    }
    
    if (wParam >= '1' && wParam <= '9') {
        // Input number 1-9, no correctness check
        int num = wParam - '0';
        sudoku->grid[g_selected_row][g_selected_col] = num;
    } else if (wParam == VK_DELETE || wParam == VK_BACK || wParam == '0') {
        // Delete number
        sudoku->grid[g_selected_row][g_selected_col] = EMPTY_CELL;
    }
}


// Initialize GUI
void InitializeGUI() {
    // Register window class
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "SudokuWindow";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    
    RegisterClassA(&wc);
    
    // Create window
    g_hWnd = CreateWindowA(
        "SudokuWindow",
        "Percent Sudoku",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );
    
    if (g_hWnd) {
        ShowWindow(g_hWnd, SW_SHOW);
        UpdateWindow(g_hWnd);
    }
}

// Parse command line arguments
void parse_command_line() {
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    
    if (argc >= 2) {
        // Convert wide string to multibyte string
        int size = WideCharToMultiByte(CP_UTF8, 0, argv[1], -1, NULL, 0, NULL, NULL);
        g_input_filename = malloc(size);
        WideCharToMultiByte(CP_UTF8, 0, argv[1], -1, g_input_filename, size, NULL, NULL);
    }
    
    LocalFree(argv);
}

// Main function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Avoid unused parameter warnings
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;
    
    // Parse command line arguments
    parse_command_line();
    
    InitializeGUI();
    
    if (!g_hWnd) {
        return -1;
    }
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}

#endif // _WIN32
