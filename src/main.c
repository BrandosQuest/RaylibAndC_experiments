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
    TfLiteInterpreterOptionsSetNumThreads(options, 4);
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

    float* input_buffer = (float*)TfLiteTensorData(input_tensor);
    int model_input_size = TfLiteTensorDim(input_tensor, 1);

    while (!WindowShouldClose()) {
        double t0 = GetTime();
        unsigned char* frameData = get_camera_frame();
        double t1 = GetTime();

        get_preprocessed_frame(input_buffer, model_input_size);
        double t2 = GetTime();

        TfLiteInterpreterInvoke(palm_interpreter);
        double t3 = GetTime();

        if (frameData != NULL) {
            UpdateTexture(camTexture, frameData);
        }

        BeginDrawing();
        if (frameData != NULL) {
            DrawTexture(camTexture, 0, 0, WHITE);
        }
        
        DrawRectangle(10, 10, 320, 110, Fade(BLACK, 0.7f));
        DrawText(TextFormat("FPS: %i", GetFPS()), 20, 20, 20, GREEN);
        DrawText(TextFormat("Cam Capture: %.1f ms", (t1 - t0) * 1000.0), 20, 45, 18, RAYWHITE);
        DrawText(TextFormat("Preprocess:  %.1f ms", (t2 - t1) * 1000.0), 20, 70, 18, RAYWHITE);
        DrawText(TextFormat("TFLite Run:  %.1f ms", (t3 - t2) * 1000.0), 20, 95, 18, YELLOW);
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