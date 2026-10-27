#include "raylib.h"
#include <time.h> // C Standard Library for real-world time
#include <stdio.h>
#include <stdlib.h>
#include "tensorflow/lite/c/c_api.h"

int main(void)
{
    // ========================================================
    // 1. INITIALIZATION
    // ========================================================
    InitWindow(800, 600, "Time Example");
    SetTargetFPS(60);

    // ========================================================
    // GAME LOOP
    // ========================================================
    while (!WindowShouldClose())
    {
        // ----------------------------------------------------
        // 2. UPDATE
        // ----------------------------------------------------
        // Raylib timing:
        double elapsedSeconds = GetTime();
        float deltaTime = GetFrameTime();
        int currentFPS = GetFPS();

        // Standard C wall-clock time:
        time_t rawTime = time(NULL);
        struct tm *info = localtime(&rawTime);

        // Format wall-clock time as HH:MM:SS
        char clockString[20];
        strftime(clockString, sizeof(clockString), "%Y-%m-%d %H:%M:%S", info);

        // ---------------------------------------------------- 
        // 3. DRAW
        // ----------------------------------------------------
        BeginDrawing();
            ClearBackground(BLACK);

            // Draw Raylib game time
            DrawText(TextFormat("Running Time: %.2f s", elapsedSeconds), 50, 50, 20, RAYWHITE);
            DrawText(TextFormat("Delta Time: %.4f s", deltaTime), 50, 80, 20, RAYWHITE);
            DrawText(TextFormat("FPS: %i", currentFPS), 50, 110, 20, GREEN);

            // Draw real-world wall clock
            DrawText(TextFormat("Current Time: %s", clockString), 50, 160, 20, YELLOW);
        EndDrawing();
    }

    // ========================================================
    // 4. DE-INITIALIZATION
    // ========================================================
    CloseWindow();
    return 0;
}