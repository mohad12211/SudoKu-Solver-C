#include <stdbool.h>

void printBoard(int board[9][9]);
void solve(int board[9][9]);
bool compare(int puzzle[9][9], int solution[9][9]);
void initBoard(int board[9][9], char *string);
bool solveRecursive(int board[9][9], int startRow, int startColumn);
bool isValid(int board[9][9]);
bool getNextEmptyCell(int board[9][9], int *currentRow, int *currentColumn);
