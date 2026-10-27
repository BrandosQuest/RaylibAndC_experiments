#include <stdio.h>
#include "raylib.h"
#include "tensorflow/lite/c/c_api.h"

int main(void) {
    // Test 1: Can we call TensorFlow Lite?
    printf("Successfully linked TensorFlow Lite version: %s\n", TfLiteVersion());

    // Test 2: Can we initialize Raylib?
    InitWindow(800, 450, "GestureSynth C - Build Test");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("TensorFlow Lite and Raylib are working!", 190, 200, 20, DARKGRAY);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}