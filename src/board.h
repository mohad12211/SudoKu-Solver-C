#include <stdbool.h>

typedef struct {
  bool pencilMarks[9];
  bool given;
  int number;
} Cell;

typedef struct {
  Cell cells[9][9];
  int solution[9][9];
  int selectedNumber;
} Board;

void setNewBoard(Board *board, int difficulty);
bool BoardIsNumberComplete(Board *b, int number);
void BoardUpdateNumber(Board *b, int number);
void BoardInit(Board *board, const char *string);

void render(void);
void printBoard(int board[9][9]);
void solve(int board[9][9]);
bool compare(int puzzle[9][9], int solution[9][9]);
bool solveRecursive(int board[9][9], int startRow, int startColumn);
bool isValid(int board[9][9]);
bool getNextEmptyCell(int board[9][9], int *currentRow, int *currentColumn);
char *getDiffFromInt(int difficulty);
