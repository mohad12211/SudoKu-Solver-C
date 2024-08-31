#define _GNU_SOURCE
#include "board.h"
#include <raylib.h>
#include <stdio.h>
#include <string.h>

// TODO: Handle invalid strings.
void BoardInit(Board *board, const char *string) {
  board->selectedNumber = -1;
  for (int row = 0; row < 9; row++) {
    for (int column = 0; column < 9; column++) {
      int number = *string++ - '0';
      Cell cell = {{0}, number != 0, number};
      board->cells[row][column] = cell;
      board->solution[row][column] = number;
    }
  }
  solve(board->solution);
}

void setNewBoard(Board *board, char *difficulty) {
  FILE *fd;
  char line[512];
  char *str;
  fd = popen(TextFormat("curl https://sudoku.com/api/v2/level/%s -H 'X-Requested-With: XMLHttpRequest' 2> /dev/null", difficulty), "r");
  fgets(line, sizeof(line), fd);
  str = strstr(line, "mission\":\"") + strlen("mission\":\"");
  str[81] = '\0';
  printf("%s\n", str);
  BoardInit(board, str);
  pclose(fd);
}

void BoardUpdateNumber(Board *b, int number) { b->selectedNumber = number; }

bool BoardIsNumberComplete(Board *b, int number) {
  int count = 0;
  for (int row = 0; row < 9; row++) {
    for (int column = 0; column < 9; column++) {
      if (b->cells[row][column].number == number) {
        count++;
      }
    }
  }
  return count == 9;
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

char *getDiffFromInt(int difficulty) {
  switch (difficulty) {
  case 0:
    return "easy";
  case 1:
    return "medium";
  case 2:
    return "hard";
  case 3:
    return "expert";
  case 4:
    return "evil";
  case 5:
    return "extreme";
  }
  return 0;
}
