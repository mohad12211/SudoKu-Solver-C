#include <stdbool.h>
#include <stdio.h>

#include "main.h"

int main(void) {

  int puzzle[9][9] = {0};
  initBoard(puzzle, "000005208300070506008420090009000003835000000060040002057800000680000009200007000");
  int solution[9][9] = {0};
  initBoard(solution, "976315248342978516518426397429681753835792461761543982157839624683254179294167835");

  solve(puzzle);
  compare(puzzle, solution);
  return 0;
}

void solve(int board[9][9]) {
  int startRow = 0;
  int startColumn = 0;
  if (!getNextEmptyCell(board, &startRow, &startColumn)) {
    printf("Puzzle already solved.\n");
    return;
  }
  if (!solveRecursive(board, startRow, startColumn)) {
    printf("Puzzle is unsolvable.\n");
    return;
  }
  printBoard(board);
}

bool solveRecursive(int board[9][9], int startRow, int startColumn) {
  int cell = board[startRow][startColumn];

  for (int testNumber = cell + 1; testNumber <= 9; testNumber++) {
    board[startRow][startColumn] = testNumber;
    if (!isValid(board))
      continue; // Board is invalid, test the next number.
    int nextRow = startRow;
    int nextColumn = startColumn;
    if (!getNextEmptyCell(board, &nextRow, &nextColumn))
      return true; // No empty cells, and the board is valid. We solved it.
    if (!solveRecursive(board, nextRow, nextColumn))
      continue; // This board is unsolvable, try the next number.
    return true;
  }

  // We tried all numbers for this cell, all boards are unsolvable. mark the cell as empty again.
  board[startRow][startColumn] = 0;
  return false;
}

bool getNextEmptyCell(int board[9][9], int *currentRow, int *currentColumn) {
  int cell = board[*currentRow][*currentColumn];
  while (cell != 0) {
    if (*currentColumn != 8) {
      (*currentColumn)++;
    } else if (*currentRow != 8) {
      (*currentRow)++;
      *currentColumn = 0;
    } else {
      return false; // No empty cells.
    }
    cell = board[*currentRow][*currentColumn];
  }
  return true;
}

bool isValid(int board[9][9]) {
  // Check the rows.
  for (int row = 0; row < 9; row++) {
    bool duplicate[10] = {0}; // Ignore the 0th index.
    for (int column = 0; column < 9; column++) {
      int cell = board[row][column];
      if (cell != 0 && duplicate[cell]) {
        return false;
      }
      duplicate[cell] = true;
    }
  }

  // Check the columns.
  for (int column = 0; column < 9; column++) {
    bool duplicate[10] = {0}; // Ignore the 0th index.
    for (int row = 0; row < 9; row++) {
      int cell = board[row][column];
      if (cell != 0 && duplicate[cell]) {
        return false;
      }
      duplicate[cell] = true;
    }
  }

  // Check 3x3 blocks.
  for (int column = 0; column < 9; column += 3) {
    for (int row = 0; row < 9; row += 3) {
      bool duplicate[10] = {0}; // Ingore the 0th index.
      for (int subcolumn = column; subcolumn < column + 3; subcolumn++) {
        for (int subrow = row; subrow < row + 3; subrow++) {
          int cell = board[subrow][subcolumn];
          if (cell != 0 && duplicate[cell]) {
            return false;
          }
          duplicate[cell] = true;
        }
      }
    }
  }

  return true;
}

// TODO: Handle invalid strings.
void initBoard(int board[9][9], char *string) {
  for (int row = 0; row < 9; row++) {
    for (int column = 0; column < 9; column++) {
      board[row][column] = *string++ - '0';
    }
  }
}

bool compare(int puzzle[9][9], int solution[9][9]) {
  for (int row = 0; row < 9; row++) {
    for (int column = 0; column < 9; column++) {
      if (puzzle[row][column] != solution[row][column]) {
        printf("The solved board DOESN'T MATCH the solution.\n");
        return false;
      }
    }
  }
  printf("The solved board MATCHES the solution.\n");
  return true;
}

void printBoard(int board[9][9]) {
  printf("-------------------------\n");
  for (int row = 0; row < 9; row++) {
    for (int column = 0; column < 9; column++) {
      if (column == 0) {
        printf("| ");
      }
      printf("%d ", board[row][column]);
      if ((column + 1) % 3 == 0) {
        printf("| ");
      }
    }
    if ((row + 1) % 3 == 0) {
      printf("\n-------------------------");
    }
    printf("\n");
  }
}
