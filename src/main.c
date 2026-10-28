#include <stdio.h>
#include "raylib.h"
#include "tensorflow/lite/c/c_api.h"
#include "camera.h"
#include "tensorflow/lite/delegates/xnnpack/xnnpack_delegate.h"
#include <math.h>

#define CAM_WIDTH 1280
#define CAM_HEIGHT 720

// #define CAM_WIDTH 640
// #define CAM_HEIGHT 480


typedef struct {
    float x;
    float y;
} Anchor;

// Generates the 2016 SSD anchors for the 192x192 MediaPipe Palm Detector
void generate_palm_anchors(Anchor* anchors) {
    int idx = 0;
    // Layer 0: 24x24 grid (stride 8), 2 anchors per cell = 1152 anchors
    for (int y = 0; y < 24; y++) {
        for (int x = 0; x < 24; x++) {
            float cx = (x + 0.5f) / 24.0f;
            float cy = (y + 0.5f) / 24.0f;
            for (int a = 0; a < 2; a++) {
                anchors[idx].x = cx;
                anchors[idx].y = cy;
                idx++;
            }
        }
    }
    // Layer 1: 12x12 grid (stride 16), 6 anchors per cell = 864 anchors
    for (int y = 0; y < 12; y++) {
        for (int x = 0; x < 12; x++) {
            float cx = (x + 0.5f) / 12.0f;
            float cy = (y + 0.5f) / 12.0f;
            for (int a = 0; a < 6; a++) {
                anchors[idx].x = cx;
                anchors[idx].y = cy;
                idx++;
            }
        }
    }
}

int main(void) {
    if (!init_camera(CAM_WIDTH, CAM_HEIGHT)) {
        printf("---INFO_CUSTOM: Failed to initialize webcam!\n");
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
        printf("---INFO_CUSTOM: Failed to load palm_detection_lite.tflite\n");
        return 1;
    }

    TfLiteInterpreterOptions* options = TfLiteInterpreterOptionsCreate();
    TfLiteInterpreterOptionsSetNumThreads(options, 4);

    // Explicitly create and attach the XNNPACK SIMD delegate
    TfLiteXNNPackDelegateOptions xnnpack_options = TfLiteXNNPackDelegateOptionsDefault();
    xnnpack_options.num_threads = 4;
    TfLiteDelegate* xnnpack_delegate = TfLiteXNNPackDelegateCreate(&xnnpack_options);
    TfLiteInterpreterOptionsAddDelegate(options, xnnpack_delegate);

    TfLiteInterpreter* palm_interpreter = TfLiteInterpreterCreate(palm_model, options);
    TfLiteInterpreterAllocateTensors(palm_interpreter);

    // Inspect Input Tensor 0
    TfLiteTensor* input_tensor = TfLiteInterpreterGetInputTensor(palm_interpreter, 0);
    printf("---INFO_CUSTOM: Palm Model Input Dimensions: [%d, %d, %d, %d] | Type: %d | Bytes: %zu\n",
        TfLiteTensorDim(input_tensor, 0), // Batch size (1)
        TfLiteTensorDim(input_tensor, 1), // Height (usually 192)
        TfLiteTensorDim(input_tensor, 2), // Width (usually 192)
        TfLiteTensorDim(input_tensor, 3), // Channels (3 - RGB)
        TfLiteTensorType(input_tensor),   // kTfLiteFloat32 (1)
        TfLiteTensorByteSize(input_tensor));


    // Inspect Output Tensors (0 = regressors, 1 = scores)
    const TfLiteTensor* out_regressors = TfLiteInterpreterGetOutputTensor(palm_interpreter, 0);
    const TfLiteTensor* out_scores = TfLiteInterpreterGetOutputTensor(palm_interpreter, 1);

    printf("---INFO_CUSTOM: Output 0 (Regressors): [%d, %d, %d]\n",
           TfLiteTensorDim(out_regressors, 0),
           TfLiteTensorDim(out_regressors, 1),
           TfLiteTensorDim(out_regressors, 2));

    printf("---INFO_CUSTOM: Output 1 (Scores):     [%d, %d, %d]\n",
           TfLiteTensorDim(out_scores, 0),
           TfLiteTensorDim(out_scores, 1),
           TfLiteTensorDim(out_scores, 2));

           

    float* input_buffer = (float*)TfLiteTensorData(input_tensor);
    int model_input_size = TfLiteTensorDim(input_tensor, 1);

    Anchor anchors[2016];
    generate_palm_anchors(anchors);

    while (!WindowShouldClose()) {
        double t0 = GetTime();
        unsigned char* frameData = get_camera_frame();
        double t1 = GetTime();

        get_preprocessed_frame(input_buffer, model_input_size);
        double t2 = GetTime();

        TfLiteInterpreterInvoke(palm_interpreter);

        const float* raw_scores = (const float*)TfLiteTensorData(out_scores);
        int num_anchors = TfLiteTensorDim(out_scores, 1); // 2016

        int best_anchor_idx = -1;
        float max_logit = -1000.0f;

        for (int i = 0; i < num_anchors; i++) {
            if (raw_scores[i] > max_logit) {
                max_logit = raw_scores[i];
                best_anchor_idx = i;
            }
        }

        // Convert raw logit to probability [0.0 - 1.0] via sigmoid
        float best_prob = 1.0f / (1.0f + expf(-max_logit));

        double t3 = GetTime();

        if (frameData != NULL) {
            UpdateTexture(camTexture, frameData);
        }

        BeginDrawing();
        if (frameData != NULL) {
            DrawTexture(camTexture, 0, 0, WHITE);
        }

        // --- Inside BeginDrawing(), after DrawTexture(camTexture, 0, 0, WHITE); ---
        if (best_prob > 0.60f && best_anchor_idx >= 0) {
            const float* reg = (const float*)TfLiteTensorData(out_regressors) + (best_anchor_idx * 18);
            
            float anchor_x = anchors[best_anchor_idx].x;
            float anchor_y = anchors[best_anchor_idx].y;

            // 1. Decode center and size in normalized [0.0 - 1.0] space of the 192x192 square
            float cx_norm = anchor_x + (reg[0] / 192.0f);
            float cy_norm = anchor_y + (reg[1] / 192.0f);
            float w_norm  = reg[2] / 192.0f;
            float h_norm  = reg[3] / 192.0f;

            // 2. Map from 192x192 padded square back to 1280x720 screen pixels
            // Since 1280x720 is 16:9, width fills the 192px square, and height is padded symmetrically
            float scale = (float)CAM_WIDTH; // 1280.0f
            float pad_y_norm = (1.0f - ((float)CAM_HEIGHT / (float)CAM_WIDTH)) * 0.5f; // ~0.21875

            float box_cx = cx_norm * scale;
            float box_cy = (cy_norm - pad_y_norm) * scale;
            float box_w  = w_norm * scale;
            float box_h  = h_norm * scale;

            // Draw the Palm Bounding Box
            DrawRectangleLines((int)(box_cx - box_w * 0.5f), 
                               (int)(box_cy - box_h * 0.5f), 
                               (int)box_w, (int)box_h, GREEN);

            // 3. Decode and draw the 7 Palm Keypoints
            for (int k = 0; k < 7; k++) {
                float kp_x_norm = anchor_x + (reg[4 + k * 2 + 0] / 192.0f);
                float kp_y_norm = anchor_y + (reg[4 + k * 2 + 1] / 192.0f);

                int screen_x = (int)(kp_x_norm * scale);
                int screen_y = (int)((kp_y_norm - pad_y_norm) * scale);

                // Draw Wrist (k=0) and Middle MCP (k=2) in YELLOW, others in RED
                Color kp_color = (k == 0 || k == 2) ? YELLOW : RED;
                DrawCircle(screen_x, screen_y, 5, kp_color);
            }
        }

        
        DrawRectangle(10, 10, 320, 135, Fade(BLACK, 0.7f));
        DrawText(TextFormat("FPS: %i", GetFPS()), 20, 20, 20, GREEN);
        DrawText(TextFormat("Cam Capture: %.1f ms", (t1 - t0) * 1000.0), 20, 45, 18, RAYWHITE);
        DrawText(TextFormat("Preprocess:  %.1f ms", (t2 - t1) * 1000.0), 20, 70, 18, RAYWHITE);
        DrawText(TextFormat("TFLite Run:  %.1f ms", (t3 - t2) * 1000.0), 20, 95, 18, YELLOW);
        Color detectColor = (best_prob > 0.6f) ? GREEN : RED;
        DrawText(TextFormat("Hand Prob: %.1f%% (Anchor %d)", best_prob * 100.0f, best_anchor_idx), 
                 20, 120, 18, detectColor);
        EndDrawing();
    }

    close_camera();
    UnloadTexture(camTexture);
    CloseWindow();

    TfLiteInterpreterDelete(palm_interpreter);
    TfLiteXNNPackDelegateDelete(xnnpack_delegate);
    TfLiteInterpreterOptionsDelete(options);
    TfLiteModelDelete(palm_model);

    return 0;
}