#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <stdbool.h>
#include "raylib.h"

Font customFont;

#define WIDTH 96
#define HEIGHT 96
#define NUM_CELLS WIDTH *HEIGHT

#define WINDOW_X 1200
#define WINDOW_Y 800
#define CELL_PADDING 1

// 20Hz
const double stepIntervalSeconds = 1.0 / 20.0;

int step;
bool paused = true;

static inline int cellIdx(int x, int y)
{
    return x * WIDTH + y;
}

typedef struct
{
    int state;
    int x;
    int y;
} Cell;

typedef struct
{
    Cell cells[NUM_CELLS];
} Map;

// Global map
Map map = {0};

void init()
{
    for (int a = 0; a < WIDTH; a++)
    {
        for (int b = 0; b < HEIGHT; b++)
        {
            map.cells[cellIdx(a, b)].state = rand() % 2 == 0 ? 1 : 0;
        }
    }
}

int countNeighbors(const Cell cells[WIDTH * HEIGHT], int x, int y)
{
    int neighbors = 0;
    for (int dx = -1; dx <= 1; dx++)
    {
        for (int dy = -1; dy <= 1; dy++)
        {
            if (dx == 0 && dy == 0)
            {
                continue;
            }

            int offsetX = x + dx;
            int offsetY = y + dy;
            if (offsetX < 0 || offsetY < 0 || offsetX >= WIDTH || offsetY >= HEIGHT)
            {
                continue;
            }

            if (cells[cellIdx(offsetX, offsetY)].state == 1)
            {
                neighbors++;
            }
        }
    }

    return neighbors;
}

static inline int nextCellState(const Cell cells[WIDTH * HEIGHT], int x, int y)
{
    int neighbors = countNeighbors(cells, x, y);
    int alive = cells[cellIdx(x, y)].state == 1;

    if (alive)
    {
        return (neighbors == 2 || neighbors == 3) ? 1 : 0;
    }
    else
    {
        return (neighbors == 3) ? 1 : 0;
    }
}

void doSimulation()
{
    step++;

    // oldMap
    Cell oldCells[WIDTH * HEIGHT];
    memcpy(oldCells, map.cells, sizeof(map.cells));

    for (int x = 0; x < WIDTH; x++)
    {
        for (int y = 0; y < HEIGHT; y++)
        {
            map.cells[cellIdx(x, y)].state = nextCellState(oldCells, x, y);
        }
    }
}

void doRendering()
{
    BeginDrawing();
    int mouseX = GetMouseX();
    int mouseY = GetMouseY();
    ClearBackground(RAYWHITE);

    // WINDOW_Y = cells*x + (cells -1 ) * CELL_PADDING
    // x = (WINDOW_Y - (cells -1) * CELL_PADDING)/ cells
    int cellHeight = ((WINDOW_Y - (HEIGHT - 1) * CELL_PADDING)) / HEIGHT;
    // int cellWidth = ((WINDOW_X - (WIDTH - 1) * CELL_PADDING)) / WIDTH;
    int cellWidth = cellHeight;
    int strideX = cellWidth + CELL_PADDING;
    int strideY = cellHeight + CELL_PADDING;

    int hoverX = -1;
    int hoverY = -1;
    if (mouseX >= 0 && mouseY >= 0)
    {
        int gridX = mouseX / strideX;
        int gridY = mouseY / strideY;
        int localX = mouseX % strideX;
        int localY = mouseY % strideY;

        if (gridX >= 0 && gridX < WIDTH && gridY >= 0 && gridY < HEIGHT &&
            localX < cellWidth && localY < cellHeight)
        {
            hoverX = gridX;
            hoverY = gridY;
        }
    }

    for (int x = 0; x < WIDTH; x++)
    {
        for (int y = 0; y < HEIGHT; y++)
        {
            int cellStartX = cellWidth * x + (CELL_PADDING * (x));
            int cellStartY = cellHeight * y + (CELL_PADDING * (y));

            Color cellColor = LIGHTGRAY;
            if (map.cells[cellIdx(x, y)].state == 1)
            {
                cellColor = BLUE;
            }
            DrawRectangle(cellStartX, cellStartY, cellHeight, cellWidth, cellColor);
        }
    }

    // HUD/debug overlay
    char hud[512];
    if (hoverX >= 0 && hoverY >= 0)
    {
        int neighbors = countNeighbors(map.cells, hoverX, hoverY);
        int nextState = nextCellState(map.cells, hoverX, hoverY);
        int curState = map.cells[cellIdx(hoverX, hoverY)].state;
        snprintf(hud, sizeof(hud),
                 "step: %d%s\nhover: (%d, %d)\nnow: %s\nneighbors: %d\nnext: %s",
                 step,
                 paused ? " (paused)" : "",
                 hoverX, hoverY,
                 curState ? "alive" : "dead",
                 neighbors,
                 nextState ? "alive" : "dead");

        int cellStartX = cellWidth * hoverX + (CELL_PADDING * (hoverX));
        int cellStartY = cellHeight * hoverY + (CELL_PADDING * (hoverY));
        DrawRectangleLines(cellStartX, cellStartY, cellWidth, cellHeight, RED);
    }
    else
    {
        snprintf(hud, sizeof(hud), "step: %d%s\nhover: (none)", step, paused ? " (paused)" : "");
    }

    DrawTextEx(customFont, hud, (Vector2){10, 10}, 20, 1, BLACK);
    EndDrawing();
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    // Init Window
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(WINDOW_X, WINDOW_Y, "Game of Life");

    init();

    // Load font
    // Try bundled font in potential paths
    if (FileExists("assets/SF-Pro.ttf"))
    {
        customFont = LoadFontEx("assets/SF-Pro.ttf", 32, 0, 250);
    }
    else
    {
        customFont = GetFontDefault();
    }

    // Fallback validation
    if (customFont.texture.id == 0)
    {
        customFont = GetFontDefault();
    }
    else
    {
        SetTextureFilter(customFont.texture, TEXTURE_FILTER_BILINEAR);
    }

    SetTargetFPS(60);

    double lastStepTime = GetTime();

    while (!WindowShouldClose())
    {
        if (paused)
        {
            if (GetKeyPressed() != 0 ||
                IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
                IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) ||
                IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE))
            {
                paused = false;
                lastStepTime = GetTime();
            }
        }

        doRendering();

        if (!paused)
        {
            double now = GetTime();
            if ((now - lastStepTime) >= stepIntervalSeconds)
            {
                doSimulation();
                lastStepTime = now;
            }
        }
    }

    // Cleanup
    if (customFont.texture.id != GetFontDefault().texture.id)
    {
        UnloadFont(customFont);
    }
    CloseWindow();

    return 0;
}
