#include "raylib.h"
#include "resource_dir.h"

int main(void)
{
    // ========================================================
    // 1. INITIALIZATION
    // ========================================================
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(800, 600, "RaylibAndC_experiments");
    SearchAndSetResourceDir("resources");

    // ========================================================
    // GAME LOOP
    // ========================================================
    while (!WindowShouldClose())
    {
        // ----------------------------------------------------
        // 2. UPDATE
        // ----------------------------------------------------


        // ----------------------------------------------------
        // 3. DRAW
        // ----------------------------------------------------
        BeginDrawing();
            ClearBackground(BLACK);
            DrawText(TextFormat("FPS: %d", GetFPS()), 10, 10, 20, GREEN);
        EndDrawing();
    }

    // ========================================================
    // 4. DE-INITIALIZATION
    // ========================================================
    CloseWindow();

    return 0;
}