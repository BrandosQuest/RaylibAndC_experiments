#include <stdio.h>
#include "raylib.h"
#include "tensorflow/lite/c/c_api.h"
#include "camera.h"

#define CAM_WIDTH 1280
#define CAM_HEIGHT 720

// #define CAM_WIDTH 640
// #define CAM_HEIGHT 480


int main(void) {
    if (!init_camera(CAM_WIDTH, CAM_HEIGHT)) {
        printf("Failed to initialize webcam!\n");
        return 1;
    }

    InitWindow(CAM_WIDTH, CAM_HEIGHT, "GestureSynth C - Webcam Test");
    SetTargetFPS(60);

    // Create a blank texture in GPU memory to hold our camera frames
    Image blankImage = GenImageColor(CAM_WIDTH, CAM_HEIGHT, BLANK);
    Texture2D camTexture = LoadTextureFromImage(blankImage);
    UnloadImage(blankImage);

    while (!WindowShouldClose()) {
        // 1. Grab the raw RGB pixel data from OpenCV
        unsigned char* frameData = get_camera_frame();
        
        // 2. Upload the new pixels to the Raylib texture on the GPU
        if (frameData != NULL) {
            UpdateTexture(camTexture, frameData);
        }

        // 3. Draw the texture to the screen
        BeginDrawing();
        ClearBackground(BLACK);
        if (frameData != NULL) {
            DrawTexture(camTexture, 0, 0, WHITE);
        } else {
            DrawText("Waiting for camera...", 10, 10, 20, RED);
        }
        EndDrawing();
    }

    close_camera();
    UnloadTexture(camTexture);
    CloseWindow();
    
    return 0;
}