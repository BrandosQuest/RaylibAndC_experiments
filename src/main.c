#include "raylib.h"
#include "resource_dir.h"

// ========================================================
// 0. CUSTOM FUNCTIONS DECLARATION
// ========================================================
#define CUSTOM_WEIRD_COLOR      CLITERAL(Color){ 100, 0, 0, 100 }         
//void UpdatePlayer(Vector2 *pos, float speed, float delta);


int main(void)
{
    // ========================================================
    // 1. INITIALIZATION
    // ========================================================
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(800, 600, "Hello Raylib");
    SearchAndSetResourceDir("resources");
	// max FPS
    //ClearWindowState(FLAG_VSYNC_HINT); 

    Texture wabbit = LoadTexture("wabbit_alpha.png");
    int i = 0;

    // ========================================================
    // GAME LOOP
    // ========================================================
    while (!WindowShouldClose())
    {
        // ----------------------------------------------------
        // 2. UPDATE
        // ----------------------------------------------------
        i++;

        // ----------------------------------------------------
        // 3. DRAW
        // ----------------------------------------------------
        BeginDrawing();
            ClearBackground(CUSTOM_WEIRD_COLOR);
			DrawText(TextFormat("Frame rate: %d", GetFPS()), 0, 0, 20, GREEN);
            DrawText("Hello Raylib", 200, 200, 20, WHITE);
            DrawTexture(wabbit, 400, 200, WHITE);
            DrawText(TextFormat("Frame: %i", i), 200, 250, 20, WHITE);
        EndDrawing();
    }

    // ========================================================
    // 4. DE-INITIALIZATION
    // ========================================================
    UnloadTexture(wabbit);
    CloseWindow();

    return 0;
}
// make
// ./bin/Debug/raylib-quickstart