#include <stdio.h>
#include "raylib.h"
#include "tensorflow/lite/c/c_api.h"
#include "camera.h"
#include "tensorflow/lite/delegates/xnnpack/xnnpack_delegate.h"
#include <math.h>

#define CAM_WIDTH 1280
#define CAM_HEIGHT 720

// Represents a predefined coordinate on the image.
// The AI doesn't guess coordinates from scratch; it looks at these anchors 
// and predicts "how far off" the hand is from this specific point.
typedef struct {
    float x;
    float y;
} Anchor;

/*
 * MediaPipe Palm Detection uses a Single Shot Detector (SSD) architecture.
 * It looks at the 192x192 image at two different "scales" (feature maps) to find large and small hands.
 * This function builds the grid of 2016 starting points (anchors) the model expects.
 */
void generate_palm_anchors(Anchor* anchors) {
    int idx = 0;
    
    // Layer 0 (Large Grid): 24x24 cells. Used to detect smaller hands.
    // Stride is 8 (192 / 24 = 8 pixels per cell). 
    // It places 2 anchors in each cell (1152 total).
    for (int y = 0; y < 24; y++) {
        for (int x = 0; x < 24; x++) {
            float cx = (x + 0.5f) / 24.0f; // Center of the cell, normalized [0.0 - 1.0]
            float cy = (y + 0.5f) / 24.0f;
            for (int a = 0; a < 2; a++) {
                anchors[idx].x = cx;
                anchors[idx].y = cy;
                idx++;
            }
        }
    }
    
    // Layer 1 (Small Grid): 12x12 cells. Used to detect larger hands covering more of the screen.
    // Stride is 16 (192 / 12 = 16 pixels per cell).
    // It places 6 anchors in each cell to handle different hand aspect ratios (864 total).
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
    // 1. Hardware Initialization
    if (!init_camera(CAM_WIDTH, CAM_HEIGHT)) {
        printf("---INFO_CUSTOM: Failed to initialize webcam!\n");
        return 1;
    }

    InitWindow(CAM_WIDTH, CAM_HEIGHT, "GestureSynth C - Webcam Test");
    SetTargetFPS(60);

    // Setup Raylib GPU texture to display the webcam feed
    Image blankImage = GenImageColor(CAM_WIDTH, CAM_HEIGHT, BLANK);
    Texture2D camTexture = LoadTextureFromImage(blankImage);
    UnloadImage(blankImage);

    // 2. TensorFlow Lite Initialization
    TfLiteModel* palm_model = TfLiteModelCreateFromFile("src/palm_detection_lite.tflite");
    if (!palm_model) {
        printf("---INFO_CUSTOM: Failed to load palm_detection_lite.tflite\n");
        return 1;
    }

    TfLiteInterpreterOptions* options = TfLiteInterpreterOptionsCreate();
    TfLiteInterpreterOptionsSetNumThreads(options, 4);

    // The XNNPACK delegate is a highly optimized math library for ARM/x86 CPUs.
    // It uses SIMD (Single Instruction, Multiple Data) to process neural networks much faster.
    TfLiteXNNPackDelegateOptions xnnpack_options = TfLiteXNNPackDelegateOptionsDefault();
    xnnpack_options.num_threads = 4;
    TfLiteDelegate* xnnpack_delegate = TfLiteXNNPackDelegateCreate(&xnnpack_options);
    TfLiteInterpreterOptionsAddDelegate(options, xnnpack_delegate);

    TfLiteInterpreter* palm_interpreter = TfLiteInterpreterCreate(palm_model, options);
    TfLiteInterpreterAllocateTensors(palm_interpreter);

    // 3. Connect Memory Pointers
    // Input is 1x192x192x3 (Batch x Height x Width x RGB)
    TfLiteTensor* input_tensor = TfLiteInterpreterGetInputTensor(palm_interpreter, 0);
    
    // Output 0 (Regressors): Shape [1, 2016, 18]. 
    // Contains the bounding box offsets and keypoints for every single anchor.
    const TfLiteTensor* out_regressors = TfLiteInterpreterGetOutputTensor(palm_interpreter, 0);
    
    // Output 1 (Scores): Shape [1, 2016, 1].
    // Contains the "confidence" that a hand exists at each of the 2016 anchors.
    const TfLiteTensor* out_scores = TfLiteInterpreterGetOutputTensor(palm_interpreter, 1);

    // We pass this buffer to OpenCV/C++ so it can write the 192x192 image directly into the AI's memory.
    float* input_buffer = (float*)TfLiteTensorData(input_tensor);
    int model_input_size = TfLiteTensorDim(input_tensor, 1); // 192

    // Pre-calculate the 2016 anchors once
    Anchor anchors[2016];
    generate_palm_anchors(anchors);

    // --- MAIN GAME LOOP ---
    while (!WindowShouldClose()) {
        double t0 = GetTime();
        
        // Grab the uncompressed 1280x720 RGB frame from the webcam
        unsigned char* frameData = get_camera_frame();
        double t1 = GetTime();

        // Process the frame (resize, add black padding to make it square, convert to float32).
        // This writes directly into the 'input_buffer' memory we got from TensorFlow.
        get_preprocessed_frame(input_buffer, model_input_size);
        double t2 = GetTime();

        // Run the neural network
        TfLiteInterpreterInvoke(palm_interpreter);

        // 4. Post-processing (Finding the Hand)
        const float* raw_scores = (const float*)TfLiteTensorData(out_scores);
        int num_anchors = TfLiteTensorDim(out_scores, 1); // 2016

        int best_anchor_idx = -1;
        float max_logit = -1000.0f;

        // Iterate through all 2016 anchors to find the one with the highest confidence score
        for (int i = 0; i < num_anchors; i++) {
            if (raw_scores[i] > max_logit) {
                max_logit = raw_scores[i];
                best_anchor_idx = i;
            }
        }

        // Neural networks output raw numbers called 'logits' which can be any negative or positive number.
        // We use the Sigmoid math function to compress this logit into a strict 0.0 to 1.0 probability.
        float best_prob = 1.0f / (1.0f + expf(-max_logit));

        double t3 = GetTime();

        // 5. Rendering
        if (frameData != NULL) {
            UpdateTexture(camTexture, frameData); // Send pixels to GPU
        }

        BeginDrawing();
        if (frameData != NULL) {
            DrawTexture(camTexture, 0, 0, WHITE); // Draw webcam background
        }

        // Only draw the tracking boxes if the AI is >60% confident it sees a hand
        if (best_prob > 0.60f && best_anchor_idx >= 0) {
            
            // Jump to the exact chunk of memory for our winning anchor.
            // Each anchor has 18 values: [cx, cy, w, h] + [7 keypoints * 2 (x,y)].
            const float* reg = (const float*)TfLiteTensorData(out_regressors) + (best_anchor_idx * 18);
            
            float anchor_x = anchors[best_anchor_idx].x;
            float anchor_y = anchors[best_anchor_idx].y;

            // -- DECODING MATH --
            // MediaPipe outputs offsets relative to the 192x192 image size.
            // We divide by 192.0f to normalize these offsets to [0.0 - 1.0] percentages.
            // We then add them to the base anchor position.
            float cx_norm = anchor_x + (reg[0] / 192.0f);
            float cy_norm = anchor_y + (reg[1] / 192.0f);
            float w_norm  = reg[2] / 192.0f;
            float h_norm  = reg[3] / 192.0f;

            // -- UNPADDING MATH --
            // Our webcam is 16:9 (1280x720), but the AI required a 1:1 square (192x192).
            // To prevent stretching the image, get_preprocessed_frame() added black bars to the top and bottom.
            // We must reverse this math to draw over the original 1280x720 video.
            float scale = (float)CAM_WIDTH; // We scale everything by 1280
            
            // Calculate how much vertical space the black bars took up. 
            // For 1280x720, the image is 720/1280 = 0.5625 height.
            // The remaining 0.4375 is black padding. We divide by 2 because padding is split top/bottom.
            float pad_y_norm = (1.0f - ((float)CAM_HEIGHT / (float)CAM_WIDTH)) * 0.5f; 

            // Convert normalized coordinates back to absolute screen pixels, subtracting the padding
            float box_cx = cx_norm * scale;
            float box_cy = (cy_norm - pad_y_norm) * scale;
            float box_w  = w_norm * scale;
            float box_h  = h_norm * scale;

            // Draw bounding box (offsetting center by half width/height to get the top-left corner)
            DrawRectangleLines((int)(box_cx - box_w * 0.5f), 
                               (int)(box_cy - box_h * 0.5f), 
                               (int)box_w, (int)box_h, GREEN);

            // Decode and draw the 7 Palm Keypoints (Wrist, knuckles, etc.)
            // The loop skips the first 4 bounding box values (reg[4 + ...])
            for (int k = 0; k < 7; k++) {
                float kp_x_norm = anchor_x + (reg[4 + k * 2 + 0] / 192.0f);
                float kp_y_norm = anchor_y + (reg[4 + k * 2 + 1] / 192.0f);

                int screen_x = (int)(kp_x_norm * scale);
                int screen_y = (int)((kp_y_norm - pad_y_norm) * scale);

                Color kp_color = (k == 0 || k == 2) ? YELLOW : RED;
                DrawCircle(screen_x, screen_y, 5, kp_color);
            }
        }
        
        // 6. On-Screen Performance Metrics
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

    // 7. Graceful Cleanup
    close_camera();
    UnloadTexture(camTexture);
    CloseWindow();

    TfLiteInterpreterDelete(palm_interpreter);
    TfLiteXNNPackDelegateDelete(xnnpack_delegate);
    TfLiteInterpreterOptionsDelete(options);
    TfLiteModelDelete(palm_model);

    return 0;
}