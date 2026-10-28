#include "camera.h"
#include <opencv2/opencv.hpp>
#include <stdio.h>

static cv::VideoCapture cap;
static cv::Mat raw_frame;
static cv::Mat frame;
static cv::Mat rgba_frame;
static cv::Mat resized_bgr;
static cv::Mat padded_bgr;
static cv::Mat padded_rgb;

extern "C" {

int init_camera(int width, int height) {
    cap.open(0, cv::CAP_V4L2);
    if (!cap.isOpened()) return 0;

    int fourcc = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
    
    // Set MJPG, then resolution, then MJPG and FPS again so V4L2 doesn't reset it
    cap.set(cv::CAP_PROP_FOURCC, fourcc);
    cap.set(cv::CAP_PROP_FRAME_WIDTH, width);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, height);
    cap.set(cv::CAP_PROP_FOURCC, fourcc);
    cap.set(cv::CAP_PROP_FPS, 30);
    cap.set(cv::CAP_PROP_BUFFERSIZE, 1);

    // Query what the Linux V4L2 driver ACTUALLY negotiated
    int actual_w = (int)cap.get(cv::CAP_PROP_FRAME_WIDTH);
    int actual_h = (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    int actual_fps = (int)cap.get(cv::CAP_PROP_FPS);
    int actual_fourcc = (int)cap.get(cv::CAP_PROP_FOURCC);

    printf("---INFO_CUSTOM: V4L2 Negotiated: %dx%d @ %d FPS | Codec: %c%c%c%c\n",
           actual_w, actual_h, actual_fps,
           actual_fourcc & 0xFF,
           (actual_fourcc >> 8) & 0xFF,
           (actual_fourcc >> 16) & 0xFF,
           (actual_fourcc >> 24) & 0xFF);

    return 1;
}

unsigned char* get_camera_frame(void) {
    if (!cap.isOpened()) return nullptr;
    
    cap >> raw_frame; // Grab a new frame from hardware
    if (raw_frame.empty()) return nullptr;
    
    // Flip horizontally ONCE here so both Raylib and the AI see the mirrored image
    cv::flip(raw_frame, frame, 1);
    
    // Convert BGR to RGBA for Raylib
    cv::cvtColor(frame, rgba_frame, cv::COLOR_BGR2RGBA);
    
    return rgba_frame.data; 
}

int get_preprocessed_frame(float* out_buffer, int target_size) {
    if (frame.empty() || out_buffer == nullptr) return 0;

    int orig_w = frame.cols;
    int orig_h = frame.rows;
    float scale = std::min((float)target_size / orig_w, (float)target_size / orig_h);
    
    int new_w = (int)(orig_w * scale);
    int new_h = (int)(orig_h * scale);

    // 1. Shrink FIRST on the 3-channel BGR image
    cv::resize(frame, resized_bgr, cv::Size(new_w, new_h), 0, 0, cv::INTER_NEAREST);

    // 2. Pad the small image with black bars to target_size x target_size (192x192)
    int pad_top = (target_size - new_h) / 2;
    int pad_bottom = target_size - new_h - pad_top;
    int pad_left = (target_size - new_w) / 2;
    int pad_right = target_size - new_w - pad_left;

    cv::copyMakeBorder(resized_bgr, padded_bgr, pad_top, pad_bottom, pad_left, pad_right, 
                       cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));

    // 3. Convert BGR -> RGB only on the tiny 192x192 image
    cv::cvtColor(padded_bgr, padded_rgb, cv::COLOR_BGR2RGB);

    // 4. Normalize uint8 [0-255] to float32 [0.0-1.0] directly into TFLite's buffer
    cv::Mat float_mat(target_size, target_size, CV_32FC3, out_buffer);
    padded_rgb.convertTo(float_mat, CV_32FC3, 1.0 / 255.0);

    return 1;
}

void close_camera(void) {
    if (cap.isOpened()) {
        cap.release();
    }
}

}