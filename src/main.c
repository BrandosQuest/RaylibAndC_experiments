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

    // Load the Palm Detection model
    // Note: Assuming you run the binary from the project root
    TfLiteModel* palm_model = TfLiteModelCreateFromFile("src/palm_detection_lite.tflite");
    if (!palm_model) {
        printf("Failed to load palm_detection_lite.tflite\n");
        return 1;
    }

    TfLiteInterpreterOptions* options = TfLiteInterpreterOptionsCreate();
    TfLiteInterpreterOptionsSetNumThreads(options, 2);
    TfLiteInterpreter* palm_interpreter = TfLiteInterpreterCreate(palm_model, options);
    TfLiteInterpreterAllocateTensors(palm_interpreter);

    // Inspect Input Tensor 0
    TfLiteTensor* input_tensor = TfLiteInterpreterGetInputTensor(palm_interpreter, 0);
    printf("Palm Model Input Dimensions: [%d, %d, %d, %d] | Type: %d | Bytes: %zu\n",
        TfLiteTensorDim(input_tensor, 0), // Batch size (1)
        TfLiteTensorDim(input_tensor, 1), // Height (usually 192)
        TfLiteTensorDim(input_tensor, 2), // Width (usually 192)
        TfLiteTensorDim(input_tensor, 3), // Channels (3 - RGB)
        TfLiteTensorType(input_tensor),   // kTfLiteFloat32 (1)
        TfLiteTensorByteSize(input_tensor));

    while (!WindowShouldClose()) {
        // 1. Grab the raw RGB pixel data from OpenCV
        unsigned char* frameData = get_camera_frame();
        
        // 2. Upload the new pixels to the Raylib texture on the GPU
        if (frameData != NULL) {
            UpdateTexture(camTexture, frameData);
        }

        // 3. Draw the texture to the screen
        BeginDrawing();
        //ClearBackground(BLACK);
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

    TfLiteInterpreterDelete(palm_interpreter);
    TfLiteInterpreterOptionsDelete(options);
    TfLiteModelDelete(palm_model);

    return 0;
}