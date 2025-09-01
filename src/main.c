#include "main.h"
#include <raylib.h>
#include <stdbool.h>
#include <string.h>

#define WIDTH 1150
#define HEIGHT 800

#define GRID_COUNT 9
#define GRID_SIZE (HEIGHT / 10.)
#define GRID_PADDING 2
#define SQUARE_SIZE (GRID_SIZE + GRID_PADDING * 2)

// #define BOARD_PADDING_X ((WIDTH - (GRID_COUNT * (GRID_SIZE + GRID_PADDING))) / 2.)
#define BOARD_PADDING_X ((HEIGHT - (GRID_COUNT * (GRID_SIZE + GRID_PADDING))) / 2.)
#define BOARD_PADDING_Y ((HEIGHT - (GRID_COUNT * (GRID_SIZE + GRID_PADDING))) / 2.)

#define NUMBER_FONT_SIZE (HEIGHT / 9.)
#define PENCIL_NUMBER_FONT_SIZE (HEIGHT / 25.)
// TODO: calculate it better
#define TIME_FONT_SIZE 70
#define TEXT_FONT_SIZE 63
#define WIN_FONT_SIZE 120
#define WIN_SUBTITLE_SIZE 40

#define COLUMN_TO_X(column) ((column) * (GRID_SIZE + GRID_PADDING) + BOARD_PADDING_X)
#define ROW_TO_Y(row) ((row) * (GRID_SIZE + GRID_PADDING) + BOARD_PADDING_Y)
#define Y_TO_ROW(y) (((y) - BOARD_PADDING_Y) / (GRID_SIZE + GRID_PADDING))
#define X_TO_COLUMN(x) (((x) - BOARD_PADDING_X) / (GRID_SIZE + GRID_PADDING))

#define SELECTED_NUMBER_COLOR ((Color){33, 33, 39, 255})
#define SELECTED_PENCIL_COLOR ((Color){42, 42, 50, 255})
#define GIVEN_CELL_COLOR ((Color){200, 200, 200, 255})
#define CORRECT_CELL_COLOR ((Color){110, 150, 205, 255})
#define NUMBER_COMPLETE_COLOR ((Color){78, 83, 92, 255})
#define WRONG_CELL_COLOR (RED)
#define BACKGROUND_COLOR ((Color){55, 55, 66, 255})
#define GRID_LINES_COLOR (BLACK)
#define WIN_COLOR ((Color){50, 205, 50, 255})
#define WIN_BACKGROUND ((Color){0, 0, 0, 180})

int main(void) {
  SetTraceLogLevel(LOG_WARNING);
  SetConfigFlags(FLAG_MSAA_4X_HINT);
  InitWindow(WIDTH, HEIGHT, "Sudoku");
  SetTargetFPS(75);
  render();
  CloseWindow();

  return 0;
}

bool IsPuzzleComplete(Board *board) {
  // Check if all cells are filled and correct
  for (int row = 0; row < 9; row++) {
    for (int column = 0; column < 9; column++) {
      Cell *cell = &board->cells[row][column];
      if (cell->number == 0 || cell->number != board->solution[row][column]) {
        return false;
      }
    }
  }
  return true;
}

void DrawWinningScreen(Font winFont, Font subtitleFont, float completionTime, int difficulty) {
  // Semi-transparent overlay
  DrawRectangle(0, 0, WIDTH, HEIGHT, WIN_BACKGROUND);

  // Main congratulations text
  const char *winText = "CONGRATULATIONS!";
  Vector2 winTextSize = MeasureTextEx(winFont, winText, WIN_FONT_SIZE, 0);
  float winTextX = (WIDTH - winTextSize.x) / 2.0f;
  float winTextY = HEIGHT / 3.0f;
  DrawTextEx(winFont, winText, (Vector2){winTextX, winTextY}, WIN_FONT_SIZE, 0, WIN_COLOR);

  // Completion message
  const char *completeText = "Puzzle Solved!";
  Vector2 completeTextSize = MeasureTextEx(subtitleFont, completeText, WIN_SUBTITLE_SIZE, 0);
  float completeTextX = (WIDTH - completeTextSize.x) / 2.0f;
  float completeTextY = winTextY + winTextSize.y + 20;
  DrawTextEx(subtitleFont, completeText, (Vector2){completeTextX, completeTextY}, WIN_SUBTITLE_SIZE, 0, CORRECT_CELL_COLOR);

  // Time and difficulty info
  int hours = (int)completionTime / 3600;
  int minutes = ((int)completionTime % 3600) / 60;
  int secs = (int)completionTime % 60;
  const char *timeText = TextFormat("Time: %02d:%02d:%02d", hours, minutes, secs);
  Vector2 timeTextSize = MeasureTextEx(subtitleFont, timeText, WIN_SUBTITLE_SIZE, 0);
  float timeTextX = (WIDTH - timeTextSize.x) / 2.0f;
  float timeTextY = completeTextY + completeTextSize.y + 15;
  DrawTextEx(subtitleFont, timeText, (Vector2){timeTextX, timeTextY}, WIN_SUBTITLE_SIZE, 0, CORRECT_CELL_COLOR);

  const char *diffText = TextFormat("Difficulty: %s", getDiffFromInt(difficulty));
  Vector2 diffTextSize = MeasureTextEx(subtitleFont, diffText, WIN_SUBTITLE_SIZE, 0);
  float diffTextX = (WIDTH - diffTextSize.x) / 2.0f;
  float diffTextY = timeTextY + timeTextSize.y + 15;
  DrawTextEx(subtitleFont, diffText, (Vector2){diffTextX, diffTextY}, WIN_SUBTITLE_SIZE, 0, CORRECT_CELL_COLOR);

  // Instructions
  const char *instructText = "Press ENTER for new puzzle or ESC to exit";
  Vector2 instructTextSize = MeasureTextEx(subtitleFont, instructText, WIN_SUBTITLE_SIZE - 10, 0);
  float instructTextX = (WIDTH - instructTextSize.x) / 2.0f;
  float instructTextY = diffTextY + diffTextSize.y + 40;
  DrawTextEx(subtitleFont, instructText, (Vector2){instructTextX, instructTextY}, WIN_SUBTITLE_SIZE - 10, 0, GIVEN_CELL_COLOR);
}

void render(void) {
  Board board = {.selectedNumber = -1};
  Font numberFont = LoadFontEx("DroidSans.ttf", NUMBER_FONT_SIZE, NULL, 0);
  Font pencilFont = LoadFontEx("DroidSans.ttf", PENCIL_NUMBER_FONT_SIZE, NULL, 0);
  Font timeFont = LoadFontEx("DroidSans.ttf", TIME_FONT_SIZE, NULL, 0);
  Font textFont = LoadFontEx("DroidSans.ttf", TEXT_FONT_SIZE, NULL, 0);
  Font winFont = LoadFontEx("DroidSans.ttf", WIN_FONT_SIZE, NULL, 0);
  Font subtitleFont = LoadFontEx("DroidSans.ttf", WIN_SUBTITLE_SIZE, NULL, 0);

  bool pencilMode = false;
  float time = 0.0;
  int difficulty = 0;
  bool puzzleCompleted = false;
  float completionTime = 0.0;

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(BACKGROUND_COLOR);

    if (GetKeyPressed() == KEY_ENTER) {
      setNewBoard(&board, difficulty + 5);
      time = 0;
      puzzleCompleted = false;
      completionTime = 0.0;
    }

    if (GetKeyPressed() == KEY_ESCAPE && puzzleCompleted) {
      break; // Exit the game
    }

    if (!puzzleCompleted) {
      // Update selected cell.
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        int selectedColumn = X_TO_COLUMN(GetMouseX());
        int selectedRow = Y_TO_ROW(GetMouseY());
        if (selectedColumn >= 10 && selectedColumn <= 12 && selectedRow <= 2 && selectedRow >= 0) {
          if (board.selectedNumber == (selectedRow * 3 + (selectedColumn - 9))) {
            BoardUpdateNumber(&board, -1);
          } else {
            BoardUpdateNumber(&board, selectedRow * 3 + (selectedColumn - 9));
          }
        } else {
          if (board.selectedNumber != -1) {
            if (selectedColumn >= 0 && selectedColumn <= 8 && selectedRow >= 0 && selectedRow <= 8) {
              Cell *cell = &board.cells[selectedRow][selectedColumn];
              if (pencilMode) {
                cell->pencilMarks[board.selectedNumber - 1] = !cell->pencilMarks[board.selectedNumber - 1];
              } else if (cell->number && !cell->given) {
                cell->number = 0;
              } else if (!cell->given) {
                cell->number = board.selectedNumber;
                bool isCorrect = cell->number == board.solution[selectedRow][selectedColumn];
                if (isCorrect) {
                  for (int row = 0; row < 9; row++) {
                    if (board.cells[row][selectedColumn].pencilMarks[board.selectedNumber - 1]) {
                      board.cells[row][selectedColumn].pencilMarks[board.selectedNumber - 1] = false;
                    }
                  }
                  for (int column = 0; column < 9; column++) {
                    if (board.cells[selectedRow][column].pencilMarks[board.selectedNumber - 1]) {
                      board.cells[selectedRow][column].pencilMarks[board.selectedNumber - 1] = false;
                    }
                  }
                  for (int subcolumn = (selectedColumn / 3) * 3; subcolumn < (selectedColumn / 3) * 3 + 3; subcolumn++) {
                    for (int subrow = (selectedRow / 3) * 3; subrow < (selectedRow / 3) * 3 + 3; subrow++) {
                      if (board.cells[subrow][subcolumn].pencilMarks[board.selectedNumber - 1]) {
                        board.cells[subrow][subcolumn].pencilMarks[board.selectedNumber - 1] = false;
                      }
                    }
                  }
                }

                // Check if puzzle is completed after placing a number
                if (IsPuzzleComplete(&board)) {
                  puzzleCompleted = true;
                  completionTime = time;
                }
              }
            }
          }
        }
      } else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        pencilMode = !pencilMode;
      }
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
      if (BoardIsNumberComplete(&board, i + 1)) {
        color = NUMBER_COMPLETE_COLOR;
      } else {
        color = CORRECT_CELL_COLOR;
      }
      if (pencilMode) {
        DrawPencilNumber(i + 1, 0 + ((int)(i / 3)), 10 + (i % 3), color, pencilFont, PENCIL_NUMBER_FONT_SIZE);
      } else {
        DrawNumber(i + 1, 0 + ((int)(i / 3)), 10 + (i % 3), color, numberFont, NUMBER_FONT_SIZE);
      }
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
          DrawNumber(cell->number, row, column, color, numberFont, NUMBER_FONT_SIZE);
        } else {
          // Draw the pencilMarks.
          for (int i = 0; i < 9; i++) {
            if (cell->pencilMarks[i]) {
              DrawPencilNumber(i + 1, row, column, CORRECT_CELL_COLOR, pencilFont, PENCIL_NUMBER_FONT_SIZE);
            }
          }
        }

        DrawGridLines(row, column);
      }
    }

    {
      if (IsWindowFocused() && !puzzleCompleted) {
        time += GetFrameTime();
      }
      int hours = (int)time / 3600;
      int minutes = ((int)time % 3600) / 60;
      int secs = (int)time % 60;
      const char *text = TextFormat("%02d:%02d:%02d", hours, minutes, secs);
      const Vector2 measure = MeasureTextEx(timeFont, text, TIME_FONT_SIZE, 0);
      const Rectangle rec = {850 + 1, 300, SQUARE_SIZE * 3 - 4, measure.y};
      DrawTextEx(timeFont, text, (Vector2){rec.x + ((rec.width - measure.x) / 2.0), 300}, TIME_FONT_SIZE, 0, CORRECT_CELL_COLOR);
      DrawRectangleLinesEx(rec, GRID_PADDING, GRID_LINES_COLOR);
    }

    for (int i = 0; i <= 5; i++) {
      const char *text = getDiffFromInt(i);
      const Vector2 measure = MeasureTextEx(textFont, text, TEXT_FONT_SIZE, 0);
      const Rectangle rec = {850 + 1, 400 + measure.y * i - i * 2, SQUARE_SIZE * 3 - 4, measure.y};
      if (!puzzleCompleted && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), rec)) {
        difficulty = i;
      }
      if (difficulty == i) {
        DrawRectangleRec(rec, SELECTED_PENCIL_COLOR);
      }
      DrawRectangleLinesEx(rec, GRID_PADDING, GRID_LINES_COLOR);
      DrawTextEx(textFont, text, (Vector2){rec.x + ((rec.width - measure.x) / 2.0), rec.y}, TEXT_FONT_SIZE, 0, CORRECT_CELL_COLOR);
    }

    // Draw winning screen if puzzle is completed
    if (puzzleCompleted) {
      DrawWinningScreen(winFont, subtitleFont, completionTime, difficulty);
    }

    EndDrawing();
  }
}

void DrawNumber(int number, int row, int column, Color color, Font font, int fontSize) {
  char text[2] = {number + '0', 0};
  int textWidth = MeasureText(text, fontSize);
  float midX = COLUMN_TO_X(column) + (SQUARE_SIZE) / 2.;
  float midY = ROW_TO_Y(row) + (SQUARE_SIZE) / 2.;
  float topLeftX = midX - textWidth / 2.;
  float topLeftY = midY - fontSize / 2.;
  // 1 has weird rendering position for some fucking reason
  if (text[0] == '1') {
    topLeftX -= 12;
  }
  DrawTextEx(font, text, (Vector2){topLeftX, topLeftY}, fontSize, 0, color);
}

void DrawPencilNumber(int number, int row, int column, Color color, Font font, int fontSize) {
  char text[2] = {number + '0', 0};
  int textWidth = MeasureText(text, fontSize);
  float subSquareSize = SQUARE_SIZE / 1.65;
  float midX = COLUMN_TO_X(column) + (subSquareSize / 2.) * (((number - 1) % 3) + 1) - 8.5;
  float midY = ROW_TO_Y(row) + (subSquareSize / 2.) * ((int)((number - 1) / 3) + 1) - 7.5;
  float topLeftX = midX - textWidth / 2.;
  float topLeftY = midY - fontSize / 2.;
  // 1 has weird rendering position for some fucking reason
  if (text[0] == '1') {
    topLeftX -= 7;
  }
  DrawTextEx(font, text, (Vector2){topLeftX, topLeftY}, fontSize, 0, color);
}

void DrawGridLines(int row, int column) {
  Rectangle cellRec = {
      COLUMN_TO_X(column),
      ROW_TO_Y(row),
      SQUARE_SIZE,
      SQUARE_SIZE,
  };
  DrawRectangleLinesEx(cellRec, GRID_PADDING, GRID_LINES_COLOR);

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
