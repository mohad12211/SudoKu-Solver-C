#include "board.h"
#include <raylib.h>
#include <stdbool.h>

void DrawGridLines(int row, int column);
void DrawPencilNumber(int number, int row, int column, Color color, Font font, int fontSize);
void DrawNumber(int number, int row, int column, Color color, Font font, int fontSize);
void render(void);
void printBoard(int board[9][9]);
void solve(int board[9][9]);
bool compare(int puzzle[9][9], int solution[9][9]);
bool solveRecursive(int board[9][9], int startRow, int startColumn);
bool isValid(int board[9][9]);
bool getNextEmptyCell(int board[9][9], int *currentRow, int *currentColumn);
bool IsPuzzleComplete(Board *board);
void DrawWinningScreen(Font winFont, Font subtitleFont, float completionTime, int difficulty);
void DrawPauseScreen(Font pauseFont, Font subtitleFont);
