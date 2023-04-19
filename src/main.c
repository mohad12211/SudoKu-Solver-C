#include "main.h"
#include "board.h"
#include <raylib.h>
#include <stdbool.h>
#include <stdio.h>

#define WIDTH 1150
#define HEIGHT 800

#define GRID_COUNT 9
#define GRID_SIZE (HEIGHT / 10.)
#define GRID_PADDING 2
#define SQUARE_SIZE (GRID_SIZE + GRID_PADDING * 2)

// #define BOARD_PADDING_X ((WIDTH - (GRID_COUNT * (GRID_SIZE + GRID_PADDING))) / 2.)
#define BOARD_PADDING_X ((HEIGHT - (GRID_COUNT * (GRID_SIZE + GRID_PADDING))) / 2.)
#define BOARD_PADDING_Y ((HEIGHT - (GRID_COUNT * (GRID_SIZE + GRID_PADDING))) / 2.)

#define FONT_SIZE (HEIGHT / 9.)
#define FONT_SIZE2 (HEIGHT / 25.)

#define COLUMN_TO_X(column) ((column) * (GRID_SIZE + GRID_PADDING) + BOARD_PADDING_X)
#define ROW_TO_Y(row) ((row) * (GRID_SIZE + GRID_PADDING) + BOARD_PADDING_Y)
#define Y_TO_ROW(y) (((y)-BOARD_PADDING_Y) / (GRID_SIZE + GRID_PADDING))
#define X_TO_COLUMN(x) (((x)-BOARD_PADDING_X) / (GRID_SIZE + GRID_PADDING))

#define SELECTED_NUMBER_COLOR ((Color){33, 33, 39, 255})
#define SELECTED_PENCIL_COLOR ((Color){42, 42, 50, 255})
#define GIVEN_CELL_COLOR ((Color){200, 200, 200, 255})
#define CORRECT_CELL_COLOR ((Color){110, 150, 205, 255})
#define NUMBER_COMPLETE_COLOR ((Color){58, 71, 99, 255})
#define WRONG_CELL_COLOR (RED)
#define BACKGROUND_COLOR ((Color){55, 55, 66, 255})
#define GRID_LINES_COLOR (BLACK)

int main(void) {
  SetTraceLogLevel(LOG_WARNING);
  InitWindow(WIDTH, HEIGHT, "Sudoku");
  SetTargetFPS(75);
  render();
  CloseWindow();

  return 0;
}

void render(void) {
  Board board = {0};
  BoardInit(&board, "500609078002008400870000009010005782000810900208000304426080007100700006005020840");
  Font font = LoadFontEx("DroidSans.ttf", FONT_SIZE, NULL, 0);
  Font font2 = LoadFontEx("DroidSans.ttf", FONT_SIZE2, NULL, 0);
  bool pencilMode = false;

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(BACKGROUND_COLOR);

    if (GetKeyPressed() == KEY_ENTER) {
      BoardInit(&board, GetClipboardText());
    }

    // Update selected cell.
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      int selectedColumn = X_TO_COLUMN(GetMouseX());
      int selectedRow = Y_TO_ROW(GetMouseY());
      if (selectedColumn >= 10 && selectedColumn <= 12 && selectedRow <= 2 && selectedRow >= 0) {
        BoardUpdateNumber(&board, selectedRow * 3 + (selectedColumn - 9));
      } else {
        if (board.selectedNumber != -1) {
          if (selectedColumn >= 0 && selectedColumn <= 8 && selectedRow >= 0 && selectedRow <= 8) {
            if (pencilMode) {
              board.cells[selectedRow][selectedColumn].pencilMarks[board.selectedNumber - 1] =
                  !board.cells[selectedRow][selectedColumn].pencilMarks[board.selectedNumber - 1];
            } else if (board.cells[selectedRow][selectedColumn].number && !board.cells[selectedRow][selectedColumn].given) {
              board.cells[selectedRow][selectedColumn].number = 0;
            } else if (!board.cells[selectedRow][selectedColumn].given) {
              board.cells[selectedRow][selectedColumn].number = board.selectedNumber;
            }
          }
        }
      }
    } else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
      pencilMode = !pencilMode;
    }

    for (int i = 0; i < 9; i++) {
      Rectangle rec1 = {
          COLUMN_TO_X(10 + (i % 3)),
          ROW_TO_Y(0 + ((int)(i / 3))),
          SQUARE_SIZE,
          SQUARE_SIZE,
      };
      if (board.selectedNumber == i + 1) {
        DrawRectangle(rec1.x, rec1.y, rec1.width, rec1.height, SELECTED_NUMBER_COLOR);
        DrawRectangleLinesEx(rec1, GRID_PADDING, BLACK);
      } else {
        DrawRectangleLinesEx(rec1, GRID_PADDING, BLACK);
      }
      Color color;
      int midX;
      int midY;
      if (BoardIsNumberComplete(&board, i + 1)) {
        color = NUMBER_COMPLETE_COLOR;
      } else {
        color = CORRECT_CELL_COLOR;
      }
      if (pencilMode) {
        float subSquareSize = SQUARE_SIZE / 1.65;
        midX = COLUMN_TO_X(10 + (i % 3)) + (subSquareSize / 2.) * ((i % 3) + 1) - 8.5;
        midY = ROW_TO_Y(0 + ((int)(i / 3))) + (subSquareSize / 2.) * ((int)(i / 3) + 1) - 7.5;
        if (i == 0) {
          midX += 5; // 1 has weird rendering position for some fucking reason
        }
      } else {
        midX = COLUMN_TO_X(10 + i % 3) + SQUARE_SIZE / 2;
        midY = ROW_TO_Y(0 + ((int)(i / 3.))) + SQUARE_SIZE / 2;
      }
      DrawNumber(i + 1, midX, midY, color, pencilMode ? font2 : font, pencilMode ? FONT_SIZE2 : FONT_SIZE);
    }

    for (int column = 0; column < GRID_COUNT; column++) {
      for (int row = 0; row < GRID_COUNT; row++) {
        Cell *cell = &board.cells[row][column];
        if (cell->number == board.selectedNumber) {
          DrawRectangle(COLUMN_TO_X(column), ROW_TO_Y(row), SQUARE_SIZE, SQUARE_SIZE, SELECTED_NUMBER_COLOR);
        } else if (!cell->number && cell->pencilMarks[board.selectedNumber - 1]) {
          DrawRectangle(COLUMN_TO_X(column), ROW_TO_Y(row), SQUARE_SIZE, SQUARE_SIZE, SELECTED_PENCIL_COLOR);
        }

        // Draw the numbers.
        if (cell->number) {
          bool isCorrect = cell->number == board.solution[row][column];
          Color color = cell->given ? GIVEN_CELL_COLOR : isCorrect ? CORRECT_CELL_COLOR : WRONG_CELL_COLOR;
          float midX = COLUMN_TO_X(column) + (SQUARE_SIZE) / 2.;
          float midY = ROW_TO_Y(row) + (SQUARE_SIZE) / 2.;
          DrawNumber(cell->number, midX, midY, color, font, FONT_SIZE);
        } else {
          // Draw the pencilMarks.
          // If you wonder why these numbers seem magical, it's because they are.
          for (int i = 0; i < 9; i++) {
            if (cell->pencilMarks[i]) {
              float subSquareSize = SQUARE_SIZE / 1.65;
              float midX = COLUMN_TO_X(column) + (subSquareSize / 2.) * ((i % 3) + 1) - 8.5;
              float midY = ROW_TO_Y(row) + (subSquareSize / 2.) * ((int)(i / 3) + 1) - 7.5;
              if (i == 0) {
                midX += 5; // 1 has weird rendering position for some fucking reason
              }
              DrawNumber(i + 1, midX, midY, CORRECT_CELL_COLOR, font2, FONT_SIZE2);
            }
          }
        }

        // Draw the grid lines.
        Rectangle rec = {
            COLUMN_TO_X(column),
            ROW_TO_Y(row),
            SQUARE_SIZE,
            SQUARE_SIZE,
        };
        DrawRectangleLinesEx(rec, GRID_PADDING, GRID_LINES_COLOR);

        // Draw thick horizontal lines between the blocks.
        if ((row + 1) % 3 == 0 && row != 8) {
          Vector2 start = {
              COLUMN_TO_X(column),
              ROW_TO_Y(row + 1),
          };
          Vector2 end = {
              COLUMN_TO_X(GRID_COUNT),
              ROW_TO_Y(row + 1),
          };
          DrawLineEx(start, end, GRID_PADDING * 2, GRID_LINES_COLOR);
        }

        // Draw thick vertical lines between the blocks.
        if ((column + 1) % 3 == 0 && column != 8) {
          Vector2 start = {
              COLUMN_TO_X(column + 1),
              ROW_TO_Y(row),
          };
          Vector2 end = {
              COLUMN_TO_X(column + 1),
              ROW_TO_Y(GRID_COUNT),
          };
          DrawLineEx(start, end, GRID_PADDING * 2, GRID_LINES_COLOR);
        }
      }
    }

    EndDrawing();
  }
}

void DrawNumber(int number, int midX, int midY, Color color, Font font, int fontSize) {
  char text[2] = {number + '0', 0};
  int textWidth = MeasureText(text, fontSize);
  float topLeftX = midX - textWidth / 2.;
  float topLeftY = midY - fontSize / 2.;
  // 1 has weird rendering position for some fucking reason
  if (text[0] == '1') {
    topLeftX -= 12;
  }
  DrawTextEx(font, text, (Vector2){topLeftX, topLeftY}, fontSize, 0, color);
}
