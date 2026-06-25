#include "main.h"
#include <math.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define MOVE_STACK_MAX 1024

typedef struct {
  int row;
  int column;
  Cell cell;    // snapshot of the cell state
  int groupId;  // entries with the same groupId are undone/redone together
} MoveEntry;

typedef struct {
  MoveEntry entries[MOVE_STACK_MAX];
  int top;     // index of next free slot (0 = empty)
  int nextGroup; // monotonically increasing group counter
} MoveStack;

static void MoveStackPush(MoveStack *s, int row, int column, Cell cell, int groupId) {
  if (s->top < MOVE_STACK_MAX) {
    s->entries[s->top++] = (MoveEntry){row, column, cell, groupId};
  }
}

// Pops all entries belonging to the same group as the top entry.
// Returns false if the stack is empty.
// The caller should process all returned entries (they share the same groupId).
static bool MoveStackPopGroup(MoveStack *s, MoveEntry out[], int *count) {
  if (s->top == 0)
    return false;
  int gid = s->entries[s->top - 1].groupId;
  *count = 0;
  while (s->top > 0 && s->entries[s->top - 1].groupId == gid) {
    out[(*count)++] = s->entries[--s->top];
  }
  return true;
}

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
#define WIN_INSTRUCTION_SIZE 30

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
#define PAUSE_COLOR ((Color){255, 215, 0, 255})
#define PAUSE_BACKGROUND ((Color){0, 0, 0, 200})

// ─── Win Animation ──────────────────────────────────────────────────────────

// Confetti particle system
#define MAX_PARTICLES 250
#define PARTICLE_SPAWN_INTERVAL 0.035f // spawn burst every ~35ms
#define PARTICLE_SPAWN_DURATION 3.5f   // keep spawning for this many seconds

// Cell ripple flash
#define FLASH_SPREAD_SPEED 5.5f // cells/second, controls how fast the wave travels
#define FLASH_DURATION 0.55f    // seconds each cell stays lit during the flash

// Overlay fade-in
#define OVERLAY_FADE_START 1.8f    // seconds after win before the overlay appears
#define OVERLAY_FADE_DURATION 1.0f // seconds for overlay to reach full opacity

typedef struct {
  Vector2 pos;
  Vector2 vel;
  Color color;
  float rot;
  float rotSpeed;
  float w, h; // rectangle dimensions
  float life; // remaining life (0 = dead)
  float maxLife;
} Particle;

typedef struct {
  bool active;
  float timer; // seconds since animation started
  Particle particles[MAX_PARTICLES];
  float spawnTimer;
  float overlayAlpha; // 0.0 (transparent) → 1.0 (fully opaque)
} WinAnimation;

static Color CONFETTI_COLORS[] = {
    {255, 87, 87, 255},  // red
    {255, 165, 0, 255},  // orange
    {255, 215, 0, 255},  // gold
    {87, 230, 87, 255},  // green
    {87, 200, 255, 255}, // sky blue
    {180, 87, 255, 255}, // purple
    {255, 87, 200, 255}, // pink
    {255, 255, 87, 255}, // yellow
};
#define CONFETTI_COLOR_COUNT 8

// Find an inactive particle slot and initialise it at the top of the screen.
static void SpawnParticle(WinAnimation *anim) {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    Particle *p = &anim->particles[i];
    if (p->life <= 0.0f) {
      p->pos = (Vector2){(float)GetRandomValue(0, WIDTH), -10.0f};
      p->vel = (Vector2){(float)GetRandomValue(-60, 60), (float)GetRandomValue(120, 240)};
      p->color = CONFETTI_COLORS[GetRandomValue(0, CONFETTI_COLOR_COUNT - 1)];
      p->rot = (float)GetRandomValue(0, 360);
      p->rotSpeed = (float)GetRandomValue(-360, 360);
      p->w = (float)GetRandomValue(6, 16);
      p->h = (float)GetRandomValue(4, 10);
      p->maxLife = (float)GetRandomValue(200, 400) / 100.0f;
      p->life = p->maxLife;
      return;
    }
  }
}

static void InitWinAnimation(WinAnimation *anim) {
  anim->active = true;
  anim->timer = 0.0f;
  anim->spawnTimer = 0.0f;
  anim->overlayAlpha = 0.0f;
  for (int i = 0; i < MAX_PARTICLES; i++) {
    anim->particles[i].life = 0.0f;
  }
}

static void UpdateWinAnimation(WinAnimation *anim, float dt) {
  if (!anim->active)
    return;

  anim->timer += dt;

  // Spawn two confetti particles per interval for visual density
  if (anim->timer < PARTICLE_SPAWN_DURATION) {
    anim->spawnTimer += dt;
    while (anim->spawnTimer >= PARTICLE_SPAWN_INTERVAL) {
      SpawnParticle(anim);
      SpawnParticle(anim);
      anim->spawnTimer -= PARTICLE_SPAWN_INTERVAL;
    }
  }

  // Integrate particles
  for (int i = 0; i < MAX_PARTICLES; i++) {
    Particle *p = &anim->particles[i];
    if (p->life <= 0.0f)
      continue;

    p->life -= dt;
    p->pos.x += p->vel.x * dt;
    p->pos.y += p->vel.y * dt;
    p->vel.y += 80.0f * dt; // gravity
    // Per-particle gentle lateral drift using a staggered sine wave
    p->vel.x += sinf(anim->timer * 3.0f + (float)i * 0.5f) * 10.0f * dt;
    p->rot += p->rotSpeed * dt;

    if (p->pos.y > HEIGHT + 20.0f) {
      p->life = 0.0f; // cull off-screen particles
    }
  }

  // Smoothly fade the win overlay in after OVERLAY_FADE_START seconds
  if (anim->timer > OVERLAY_FADE_START) {
    float t = (anim->timer - OVERLAY_FADE_START) / OVERLAY_FADE_DURATION;
    if (t > 1.0f)
      t = 1.0f;
    anim->overlayAlpha = t;
  }
}

// Gold sine-wave ripple emanating outward from the center cell (4,4)
static void DrawCellFlashes(WinAnimation *anim) {
  if (!anim->active)
    return;

  for (int row = 0; row < 9; row++) {
    for (int col = 0; col < 9; col++) {
      float dr = (float)(row - 4);
      float dc = (float)(col - 4);
      float dist = sqrtf(dr * dr + dc * dc);
      float delay = dist / FLASH_SPREAD_SPEED;
      float t = anim->timer - delay;

      if (t < 0.0f || t > FLASH_DURATION)
        continue;

      // Sine envelope → smooth ramp-up and ramp-down
      float alpha = sinf((t / FLASH_DURATION) * PI);
      Color flash = {255, 215, 80, (unsigned char)(alpha * 210.0f)};
      DrawRectangle((int)COLUMN_TO_X(col), (int)ROW_TO_Y(row), (int)SQUARE_SIZE, (int)SQUARE_SIZE, flash);
    }
  }
}

// Rotate-and-fade confetti rectangles
static void DrawConfetti(WinAnimation *anim) {
  if (!anim->active)
    return;

  for (int i = 0; i < MAX_PARTICLES; i++) {
    Particle *p = &anim->particles[i];
    if (p->life <= 0.0f)
      continue;

    // Fade alpha out in the final 25 % of each particle's life
    float lifeRatio = p->life / p->maxLife;
    unsigned char alpha = (lifeRatio < 0.25f) ? (unsigned char)(lifeRatio / 0.25f * 255.0f) : 255;
    Color c = {p->color.r, p->color.g, p->color.b, alpha};

    Rectangle rect = {p->pos.x, p->pos.y, p->w, p->h};
    Vector2 origin = {p->w / 2.0f, p->h / 2.0f};
    DrawRectanglePro(rect, origin, p->rot, c);
  }
}

// ─── Best Times ─────────────────────────────────────────────────────────────

#define BEST_TIMES_APP_DIR "sudoku"
#define BEST_TIMES_FILENAME "best_times.bin"

// Returns the config dir path ("$XDG_CONFIG_HOME/sudoku" or
// "$HOME/.config/sudoku"). Caller must free().
static char *GetConfigDir(void) {
  const char *xdg = getenv("XDG_CONFIG_HOME");
  char *dir;
  if (xdg && xdg[0] != '\0') {
    int len = snprintf(NULL, 0, "%s/%s", xdg, BEST_TIMES_APP_DIR) + 1;
    dir = malloc(len);
    snprintf(dir, len, "%s/%s", xdg, BEST_TIMES_APP_DIR);
  } else {
    const char *home = getenv("HOME");
    if (!home)
      home = ".";
    int len = snprintf(NULL, 0, "%s/.config/%s", home, BEST_TIMES_APP_DIR) + 1;
    dir = malloc(len);
    snprintf(dir, len, "%s/.config/%s", home, BEST_TIMES_APP_DIR);
  }
  return dir;
}

// Returns full path to the best-times file. Caller must free().
static char *GetBestTimesPath(void) {
  char *dir = GetConfigDir();
  int len = snprintf(NULL, 0, "%s/%s", dir, BEST_TIMES_FILENAME) + 1;
  char *path = malloc(len);
  snprintf(path, len, "%s/%s", dir, BEST_TIMES_FILENAME);
  free(dir);
  return path;
}

void LoadBestTimes(float bestTimes[6]) {
  for (int i = 0; i < 6; i++)
    bestTimes[i] = 0.0f; // 0 means no record yet
  char *path = GetBestTimesPath();
  FILE *f = fopen(path, "rb");
  free(path);
  if (!f)
    return;
  fread(bestTimes, sizeof(float), 6, f);
  fclose(f);
}

void SaveBestTimes(float bestTimes[6]) {
  // Ensure the config directory exists before writing
  char *dir = GetConfigDir();
  mkdir(dir, 0755); // no-op if already present; we ignore the return value
  free(dir);

  char *path = GetBestTimesPath();
  FILE *f = fopen(path, "wb");
  free(path);
  if (!f)
    return;
  fwrite(bestTimes, sizeof(float), 6, f);
  fclose(f);
}

// ─── Main ───────────────────────────────────────────────────────────────────

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

void DrawPauseScreen(Font pauseFont, Font subtitleFont) {
  // Semi-transparent overlay
  DrawRectangle(0, 0, WIDTH, HEIGHT, PAUSE_BACKGROUND);

  // Main pause text
  const char *pauseText = "GAME PAUSED";
  Vector2 pauseTextSize = MeasureTextEx(pauseFont, pauseText, WIN_FONT_SIZE, 0);
  float pauseTextX = (WIDTH - pauseTextSize.x) / 2.0f;
  float pauseTextY = HEIGHT / 2.5f;
  DrawTextEx(pauseFont, pauseText, (Vector2){pauseTextX, pauseTextY}, WIN_FONT_SIZE, 0, PAUSE_COLOR);

  // Instructions
  const char *instructText = "Click on the window to resume";
  Vector2 instructTextSize = MeasureTextEx(subtitleFont, instructText, WIN_SUBTITLE_SIZE, 0);
  float instructTextX = (WIDTH - instructTextSize.x) / 2.0f;
  float instructTextY = pauseTextY + pauseTextSize.y + 30;
  DrawTextEx(subtitleFont, instructText, (Vector2){instructTextX, instructTextY}, WIN_SUBTITLE_SIZE, 0, CORRECT_CELL_COLOR);
}

// alpha: 0.0 (fully transparent) → 1.0 (fully opaque). Used for fade-in.
void DrawWinningScreen(Font winFont, Font subtitleFont, Font instructFont, float completionTime, int difficulty, float alpha,
                       bool newRecord) {
  // Semi-transparent darkening overlay, faded in
  DrawRectangle(0, 0, WIDTH, HEIGHT, Fade((Color){0, 0, 0, 180}, alpha));

  // Main congratulations text
  const char *winText = "CONGRATULATIONS!";
  Vector2 winTextSize = MeasureTextEx(winFont, winText, WIN_FONT_SIZE, 0);
  float winTextX = (WIDTH - winTextSize.x) / 2.0f;
  float winTextY = HEIGHT / 4.5f;
  DrawTextEx(winFont, winText, (Vector2){winTextX, winTextY}, WIN_FONT_SIZE, 0, Fade(WIN_COLOR, alpha));

  // NEW RECORD banner
  if (newRecord) {
    const char *recordText = "« NEW RECORD! »";
    Vector2 recordSize = MeasureTextEx(subtitleFont, recordText, WIN_SUBTITLE_SIZE, 0);
    float recordX = (WIDTH - recordSize.x) / 2.0f;
    float recordY = winTextY + winTextSize.y + 10;
    // Pulsing gold glow: oscillate alpha slightly for a shimmering effect
    float pulse = 0.85f + 0.15f * sinf(GetTime() * 6.0f);
    Color recordColor = Fade((Color){255, 215, 0, 255}, alpha * pulse);
    DrawTextEx(subtitleFont, recordText, (Vector2){recordX, recordY}, WIN_SUBTITLE_SIZE, 0, recordColor);
    winTextY = recordY; // shift the rest of the layout down
    winTextSize.y = recordSize.y;
  }

  // Completion message
  const char *completeText = "Puzzle Solved!";
  Vector2 completeTextSize = MeasureTextEx(subtitleFont, completeText, WIN_SUBTITLE_SIZE, 0);
  float completeTextX = (WIDTH - completeTextSize.x) / 2.0f;
  float completeTextY = winTextY + winTextSize.y + 20;
  DrawTextEx(subtitleFont, completeText, (Vector2){completeTextX, completeTextY}, WIN_SUBTITLE_SIZE, 0, Fade(CORRECT_CELL_COLOR, alpha));

  // Time and difficulty info
  int hours = (int)completionTime / 3600;
  int minutes = ((int)completionTime % 3600) / 60;
  int secs = (int)completionTime % 60;
  const char *timeText = TextFormat("Time: %02d:%02d:%02d", hours, minutes, secs);
  Vector2 timeTextSize = MeasureTextEx(subtitleFont, timeText, WIN_SUBTITLE_SIZE, 0);
  float timeTextX = (WIDTH - timeTextSize.x) / 2.0f;
  float timeTextY = completeTextY + completeTextSize.y + 15;
  DrawTextEx(subtitleFont, timeText, (Vector2){timeTextX, timeTextY}, WIN_SUBTITLE_SIZE, 0, Fade(CORRECT_CELL_COLOR, alpha));

  const char *diffText = TextFormat("Difficulty: %s", getDiffFromInt(difficulty));
  Vector2 diffTextSize = MeasureTextEx(subtitleFont, diffText, WIN_SUBTITLE_SIZE, 0);
  float diffTextX = (WIDTH - diffTextSize.x) / 2.0f;
  float diffTextY = timeTextY + timeTextSize.y + 15;
  DrawTextEx(subtitleFont, diffText, (Vector2){diffTextX, diffTextY}, WIN_SUBTITLE_SIZE, 0, Fade(CORRECT_CELL_COLOR, alpha));

  // Instructions
  const char *instructText = "Press ENTER for new puzzle or ESC to exit";
  Vector2 instructTextSize = MeasureTextEx(instructFont, instructText, WIN_INSTRUCTION_SIZE, 0);
  float instructTextX = (WIDTH - instructTextSize.x) / 2.0f;
  float instructTextY = diffTextY + diffTextSize.y + 40;
  DrawTextEx(instructFont, instructText, (Vector2){instructTextX, instructTextY}, WIN_INSTRUCTION_SIZE, 0, Fade(GIVEN_CELL_COLOR, alpha));
}

void render(void) {
  Board board = {.selectedNumber = -1};
  Font numberFont = LoadFontEx("DroidSans.ttf", NUMBER_FONT_SIZE, NULL, 0);
  Font pencilFont = LoadFontEx("DroidSans.ttf", PENCIL_NUMBER_FONT_SIZE, NULL, 0);
  Font timeFont = LoadFontEx("DroidSans.ttf", TIME_FONT_SIZE, NULL, 0);
  Font textFont = LoadFontEx("DroidSans.ttf", TEXT_FONT_SIZE, NULL, 0);
  Font winFont = LoadFontEx("DroidSans.ttf", WIN_FONT_SIZE, NULL, 0);
  // Load subtitleFont with extended codepoints so non-ASCII glyphs render correctly.
  // U+00AB «  and U+00BB »  are in the Latin-1 Supplement — guaranteed to be in DroidSans.
  int subtitleCPs[97];
  for (int i = 0; i < 95; i++)
    subtitleCPs[i] = 32 + i;
  subtitleCPs[95] = 0x00AB; // « LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
  subtitleCPs[96] = 0x00BB; // » RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
  Font subtitleFont = LoadFontEx("DroidSans.ttf", WIN_SUBTITLE_SIZE, subtitleCPs, 97);
  Font instructFont = LoadFontEx("DroidSans.ttf", WIN_INSTRUCTION_SIZE, NULL, 0);

  bool pencilMode = false;
  float time = 0.0;
  int difficulty = 0;
  bool puzzleCompleted = false;
  float completionTime = 0.0;
  bool isWindowFocused = true;
  bool isNewRecord = false;
  float bestTimes[6];
  LoadBestTimes(bestTimes);
  MoveStack undoStack = {0};
  MoveStack redoStack = {0};
  WinAnimation winAnim = {0};

  while (!WindowShouldClose()) {
    float dt = GetFrameTime();

    // Check if window focus changed
    bool currentlyFocused = IsWindowFocused();
    if (currentlyFocused != isWindowFocused) {
      isWindowFocused = currentlyFocused;
    }

    // Advance the win animation before drawing
    UpdateWinAnimation(&winAnim, dt);

    BeginDrawing();
    ClearBackground(BACKGROUND_COLOR);

    if (GetKeyPressed() == KEY_ENTER) {
      setNewBoard(&board, difficulty + 4);
      time = 0;
      puzzleCompleted = false;
      completionTime = 0.0;
      undoStack.top = 0; // clear undo/redo history on new puzzle
      redoStack.top = 0;
      winAnim.active = false; // reset animation for the new puzzle
    }

    if (GetKeyPressed() == KEY_ESCAPE && puzzleCompleted) {
      break; // Exit the game
    }

    if (!puzzleCompleted && isWindowFocused) {
      // Undo: Ctrl+Z
      if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_Z)) {
        MoveEntry group[MOVE_STACK_MAX];
        int count = 0;
        if (MoveStackPopGroup(&undoStack, group, &count)) {
          // Push current states to redo stack (same group id, reversed order)
          int redoGid = redoStack.nextGroup++;
          for (int gi = count - 1; gi >= 0; gi--) {
            MoveStackPush(&redoStack, group[gi].row, group[gi].column,
                          board.cells[group[gi].row][group[gi].column], redoGid);
          }
          // Restore all cells in the group
          for (int gi = 0; gi < count; gi++) {
            board.cells[group[gi].row][group[gi].column] = group[gi].cell;
          }
        }
      }

      // Redo: Ctrl+Y
      if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_Y)) {
        MoveEntry group[MOVE_STACK_MAX];
        int count = 0;
        if (MoveStackPopGroup(&redoStack, group, &count)) {
          // Push current states to undo stack (same group id, reversed order)
          int undoGid = undoStack.nextGroup++;
          for (int gi = count - 1; gi >= 0; gi--) {
            MoveStackPush(&undoStack, group[gi].row, group[gi].column,
                          board.cells[group[gi].row][group[gi].column], undoGid);
          }
          // Re-apply all cells in the group
          for (int gi = 0; gi < count; gi++) {
            board.cells[group[gi].row][group[gi].column] = group[gi].cell;
          }
        }
      }

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
                redoStack.top = 0; // new action clears redo history
                MoveStackPush(&undoStack, selectedRow, selectedColumn, *cell, undoStack.nextGroup++);
                cell->pencilMarks[board.selectedNumber - 1] = !cell->pencilMarks[board.selectedNumber - 1];
              } else if (cell->number && !cell->given) {
                redoStack.top = 0; // new action clears redo history
                MoveStackPush(&undoStack, selectedRow, selectedColumn, *cell, undoStack.nextGroup++);
                cell->number = 0;
              } else if (!cell->given) {
                redoStack.top = 0; // new action clears redo history
                // Assign one group id for this entire placement (number + pencil erasures)
                int gid = undoStack.nextGroup++;
                bool isCorrect = board.selectedNumber == board.solution[selectedRow][selectedColumn];
                // Save pencil-marked cells that will be erased (push them first so the
                // placed-number cell is on top and popped last, maintaining order).
                if (isCorrect) {
                  // Use a small boolean grid to avoid double-pushing the same cell
                  bool pushed[9][9] = {{false}};
                  // Same column
                  for (int row = 0; row < 9; row++) {
                    if (row != selectedRow &&
                        board.cells[row][selectedColumn].pencilMarks[board.selectedNumber - 1]) {
                      MoveStackPush(&undoStack, row, selectedColumn,
                                    board.cells[row][selectedColumn], gid);
                      pushed[row][selectedColumn] = true;
                    }
                  }
                  // Same row
                  for (int col = 0; col < 9; col++) {
                    if (col != selectedColumn &&
                        board.cells[selectedRow][col].pencilMarks[board.selectedNumber - 1] &&
                        !pushed[selectedRow][col]) {
                      MoveStackPush(&undoStack, selectedRow, col,
                                    board.cells[selectedRow][col], gid);
                      pushed[selectedRow][col] = true;
                    }
                  }
                  // Same 3x3 box
                  for (int subrow = (selectedRow / 3) * 3; subrow < (selectedRow / 3) * 3 + 3; subrow++) {
                    for (int subcol = (selectedColumn / 3) * 3; subcol < (selectedColumn / 3) * 3 + 3; subcol++) {
                      if ((subrow != selectedRow || subcol != selectedColumn) &&
                          board.cells[subrow][subcol].pencilMarks[board.selectedNumber - 1] &&
                          !pushed[subrow][subcol]) {
                        MoveStackPush(&undoStack, subrow, subcol,
                                      board.cells[subrow][subcol], gid);
                        pushed[subrow][subcol] = true;
                      }
                    }
                  }
                }
                // Save the target cell itself (placed last = popped first on undo)
                MoveStackPush(&undoStack, selectedRow, selectedColumn, *cell, gid);
                // Now apply the changes
                cell->number = board.selectedNumber;
                if (isCorrect) {
                  for (int row = 0; row < 9; row++) {
                    board.cells[row][selectedColumn].pencilMarks[board.selectedNumber - 1] = false;
                  }
                  for (int column = 0; column < 9; column++) {
                    board.cells[selectedRow][column].pencilMarks[board.selectedNumber - 1] = false;
                  }
                  for (int subcolumn = (selectedColumn / 3) * 3; subcolumn < (selectedColumn / 3) * 3 + 3; subcolumn++) {
                    for (int subrow = (selectedRow / 3) * 3; subrow < (selectedRow / 3) * 3 + 3; subrow++) {
                      board.cells[subrow][subcolumn].pencilMarks[board.selectedNumber - 1] = false;
                    }
                  }
                }

                // Check if puzzle is completed after placing a number
                if (IsPuzzleComplete(&board)) {
                  puzzleCompleted = true;
                  completionTime = time;
                  // Check & update best time for this difficulty
                  if (bestTimes[difficulty] <= 0.0f || completionTime < bestTimes[difficulty]) {
                    bestTimes[difficulty] = completionTime;
                    SaveBestTimes(bestTimes);
                    isNewRecord = true;
                  } else {
                    isNewRecord = false;
                  }
                  InitWinAnimation(&winAnim); // kick off the win animation
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

    // ── Win animation layers ──────────────────────────────────────────────
    // 1. Gold ripple flash on each cell (drawn above numbers, below confetti)
    DrawCellFlashes(&winAnim);
    // 2. Confetti rain (drawn above grid, below the darkened overlay)
    DrawConfetti(&winAnim);
    // ─────────────────────────────────────────────────────────────────────

    // ── Right-panel: timer + best time box + difficulty buttons ────────────
    // All three share the same X and width for visual symmetry.
    const float panelX = 850 + 1;
    const float panelW = SQUARE_SIZE * 3 - 4;

    // 1. Timer box
    {
      if (isWindowFocused && !puzzleCompleted) {
        time += dt;
      }
      int hours = (int)time / 3600;
      int minutes = ((int)time % 3600) / 60;
      int secs = (int)time % 60;
      const char *text = TextFormat("%02d:%02d:%02d", hours, minutes, secs);
      const Vector2 measure = MeasureTextEx(timeFont, text, TIME_FONT_SIZE, 0);
      const Rectangle rec = {panelX, 300, panelW, measure.y};
      DrawTextEx(timeFont, text, (Vector2){rec.x + (rec.width - measure.x) / 2.0f, rec.y}, TIME_FONT_SIZE, 0, CORRECT_CELL_COLOR);
      DrawRectangleLinesEx(rec, GRID_PADDING, GRID_LINES_COLOR);
    }

    // 2. Best-time box — same width & border style as the timer, flush below it
    //    MeasureTextEx for TIME_FONT_SIZE is ~70 px; timer ends at 300+70=370.
    //    Best-time box sits at 370+6=376 with height matching instructFont.
    {
      const char *label;
      Color labelColor;
      if (bestTimes[difficulty] > 0.0f) {
        int bh = (int)bestTimes[difficulty] / 3600;
        int bm = ((int)bestTimes[difficulty] % 3600) / 60;
        int bs = (int)bestTimes[difficulty] % 60;
        label = TextFormat("Best: %02d:%02d:%02d", bh, bm, bs);
        labelColor = (Color){255, 215, 0, 255}; // gold
      } else {
        label = "No record yet";
        labelColor = GIVEN_CELL_COLOR;
      }
      Vector2 measure = MeasureTextEx(instructFont, label, WIN_INSTRUCTION_SIZE, 0);
      const Rectangle rec = {panelX, 376, panelW, measure.y + 4};
      DrawRectangleLinesEx(rec, GRID_PADDING, GRID_LINES_COLOR);
      DrawTextEx(instructFont, label, (Vector2){rec.x + (rec.width - measure.x) / 2.0f, rec.y + (rec.height - measure.y) / 2.0f},
                 WIN_INSTRUCTION_SIZE, 0, labelColor);
    }

    // 3. Difficulty buttons — start just below the best-time box (376+34+6 = 416)
    for (int i = 0; i <= 5; i++) {
      const char *diffName = getDiffFromInt(i);
      const Vector2 measure = MeasureTextEx(textFont, diffName, TEXT_FONT_SIZE, 0);
      const Rectangle rec = {panelX, 416 + measure.y * i - i * 2, panelW, measure.y};
      if (!puzzleCompleted && isWindowFocused && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
          CheckCollisionPointRec(GetMousePosition(), rec)) {
        difficulty = i;
      }
      if (difficulty == i) {
        DrawRectangleRec(rec, SELECTED_PENCIL_COLOR);
      }
      DrawRectangleLinesEx(rec, GRID_PADDING, GRID_LINES_COLOR);
      DrawTextEx(textFont, diffName, (Vector2){rec.x + ((rec.width - measure.x) / 2.0), rec.y}, TEXT_FONT_SIZE, 0, CORRECT_CELL_COLOR);
    }

    // Draw winning screen if puzzle is completed (overlay fades in via winAnim.overlayAlpha)
    if (puzzleCompleted) {
      DrawWinningScreen(winFont, subtitleFont, instructFont, completionTime, difficulty, winAnim.overlayAlpha, isNewRecord);
    }
    // Draw pause screen if window is not focused and puzzle is not completed
    else if (!isWindowFocused) {
      DrawPauseScreen(winFont, subtitleFont);
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
