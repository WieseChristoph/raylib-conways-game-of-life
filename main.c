#include "raylib.h"
#include <stdio.h>

#include "rlgl.h"
#include "raymath.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
#define FPS                60
#define SQUARE_SIZE        32
#define SQUARES_X          20
#define SQUARES_Y          20
#define UPDATES_PER_SECOND 2

//----------------------------------------------------------------------------------
// Local Variables Definition (local to this module)
//----------------------------------------------------------------------------------
Camera2D camera = { 0 };
const int screenWidth = 800;
const int screenHeight = 450;
int grid[SQUARES_X][SQUARES_Y] = { 0 };
int paused = 1;

//----------------------------------------------------------------------------------
// Local Functions Declaration
//----------------------------------------------------------------------------------
static void UpdateDrawFrame(void);
static int getLiveNeighborCount(int x, int y);

//----------------------------------------------------------------------------------
// Main entry point
//----------------------------------------------------------------------------------
int main() {
    // Initialization
    InitWindow(screenWidth, screenHeight, "Conway's Game of Life");

    camera.zoom = 1.0f;

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, FPS, 1);
#else
    SetTargetFPS(FPS);

    // Main game loop
    while (!WindowShouldClose()) {
        // Update and Draw
        UpdateDrawFrame();
    }
#endif

    // De-Initialization
    CloseWindow();

    return 0;
}

// Update and draw game frame
static void UpdateDrawFrame(void) {
    // Update
    //----------------------------------------------------------------------------------
    // Get the world point that is under the mouse
    Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);

    // Translate based on mouse right click
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        Vector2 delta = GetMouseDelta();
        delta = Vector2Scale(delta, -1.0f/camera.zoom);

        camera.target = Vector2Add(camera.target, delta);
    }

    // Zoom based on mouse wheel
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        // Set the offset to where the mouse is
        camera.offset = GetMousePosition();

        // Set the target to match, so that the camera maps the world space point 
        // under the cursor to the screen space point under the cursor at any zoom
        camera.target = mouseWorldPos;

        // Zoom increment
        const float zoomIncrement = 0.125f;

        camera.zoom += (wheel*zoomIncrement);
        if (camera.zoom < zoomIncrement) camera.zoom = zoomIncrement;
    }

    // Pause the simulation
    if (IsKeyPressed(KEY_SPACE)) paused = !paused;

    // Toggle grid squares based on mouse left click if paused
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && paused) {
        // If the mouse is outside the grid, ignore it
        if (mouseWorldPos.x > 0 && mouseWorldPos.x < SQUARE_SIZE*SQUARES_X &&
            mouseWorldPos.y > 0 && mouseWorldPos.y < SQUARE_SIZE*SQUARES_Y) {
            // Get the grid position under the mouse
            int x = (int)mouseWorldPos.x/SQUARE_SIZE;
            int y = (int)mouseWorldPos.y/SQUARE_SIZE;

            // Toggle the grid square
            grid[x][y] = !grid[x][y];
        }
    }

    // Clear the grid with "c" if paused
    if (IsKeyPressed(KEY_C) && paused) {
        for (int x = 0; x < SQUARES_X; x++) {
            for (int y = 0; y < SQUARES_Y; y++) {
                grid[x][y] = 0;
            }
        }
    }

    // Update the simulation at a fixed rate
    static int updates = 0;
    if (updates++ >= FPS/UPDATES_PER_SECOND) {
        // Conway's Game of Life rules
        if (!paused) {
            int newGrid[SQUARES_X][SQUARES_Y] = { 0 };
            for (int x = 0; x < SQUARES_X; x++) {
                for (int y = 0; y < SQUARES_Y; y++) {
                    int liveNeighbors = getLiveNeighborCount(x, y);
                    // #1
                    // Any live cell with fewer than two live neighbours dies
                    if (liveNeighbors < 2) newGrid[x][y] = 0;
                    // #2
                    // Any live cell with two or three live neighbours lives on to the next generation
                    if (liveNeighbors == 2 || liveNeighbors == 3) newGrid[x][y] = grid[x][y];
                    // #3
                    // Any live cell with more than three live neighbours dies
                    if (liveNeighbors > 3) newGrid[x][y] = 0;
                    // #4
                    // Any dead cell with exactly three live neighbours becomes a live cell
                    if (liveNeighbors == 3 && grid[x][y] == 0) newGrid[x][y] = 1;
                }
            }

            // Copy the new grid to the old grid
            for (int x = 0; x < SQUARES_X; x++) {
                for (int y = 0; y < SQUARES_Y; y++) {
                    grid[x][y] = newGrid[x][y];
                }
            }
        }

        updates = 0;
    }

    //----------------------------------------------------------------------------------

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();
        ClearBackground(BLACK);

        BeginMode2D(camera);
            // Draw vertical grid lines
            for (int i = 0; i <= SQUARES_X; i++) {
            DrawLineV((Vector2){SQUARE_SIZE * i, 0}, (Vector2){SQUARE_SIZE * i, SQUARE_SIZE * SQUARES_Y}, LIGHTGRAY);
            }

            // Draw horizontal grid lines
            for (int i = 0; i <= SQUARES_Y; i++) {
                DrawLineV((Vector2){0, SQUARE_SIZE*i}, (Vector2){SQUARE_SIZE*SQUARES_X, SQUARE_SIZE*i}, LIGHTGRAY);
            }

            // Draw grid
            for (int x = 0; x < SQUARES_X; x++) {
                for (int y = 0; y < SQUARES_Y; y++) {
                    if (grid[x][y] == 1) {
                        DrawRectangle(x*SQUARE_SIZE, y*SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE, WHITE);
                    }
                }
            }
        EndMode2D();

        DrawText("Mouse right button drag to move, mouse wheel to zoom,\nleft click to toggle squares, space to play/pause,\n\"c\" to clear all squares", 10, 10, 20, WHITE);
        DrawText(paused ? "PAUSED" : "PLAYING", 10, screenHeight - 30, 20, paused ? RED : GREEN);

    EndDrawing();
    //----------------------------------------------------------------------------------
}

static int getLiveNeighborCount(int x, int y) {
    int count = 0;

    // Check the 8 neighboring squares
    if (x > 0 && y > 0) count += grid[x-1][y-1]; // Top left
    if (y > 0) count += grid[x][y-1]; // Top
    if (x < SQUARES_X-1 && y > 0) count += grid[x+1][y-1]; // Top right
    if (x > 0) count += grid[x-1][y]; // Left
    if (x < SQUARES_X-1) count += grid[x+1][y]; // Right
    if (x > 0 && y < SQUARES_Y-1) count += grid[x-1][y+1]; // Bottom left
    if (y < SQUARES_Y-1) count += grid[x][y+1]; // Bottom
    if (x < SQUARES_X-1 && y < SQUARES_Y-1) count += grid[x+1][y+1]; // Bottom right

    return count;
}
