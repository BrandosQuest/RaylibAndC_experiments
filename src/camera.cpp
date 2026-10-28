#include "camera.h"
#include <opencv2/opencv.hpp>

// Static instances hide the C++ objects from your C code
static cv::VideoCapture cap;
static cv::Mat frame;
static cv::Mat rgb_frame;
static cv::Mat flipped_rgb_frame;
static cv::Mat resized_bgr;
static cv::Mat padded_bgr;
static cv::Mat padded_rgb;

extern "C" {

int init_camera(int width, int height) {
    cap.open(0, cv::CAP_V4L2);
    if (!cap.isOpened()) return 0;

    // Force MJPG and 30 FPS
    cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
    cap.set(cv::CAP_PROP_FRAME_WIDTH, width);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, height);
    cap.set(cv::CAP_PROP_FPS, 30);
    // Reduce internal driver buffer to 1 frame so you don't get latency lag
    cap.set(cv::CAP_PROP_BUFFERSIZE, 1);

    // // Prevent the webcam from dropping below 30 FPS in indoor lighting
    // cap.set(cv::CAP_PROP_AUTO_EXPOSURE, 3); // 1 = Manual mode on V4L2 (3 = Auto)
    // cap.set(cv::CAP_PROP_EXPOSURE, 150);    // Fixed fast shutter speed

    return 1;
}

unsigned char* get_camera_frame(void) {
    if (!cap.isOpened()) return nullptr;
    
    cap >> frame; // Grab a new frame
    if (frame.empty()) return nullptr;
    
    // Convert BGR to RGB for Raylib and TFLite
    cv::cvtColor(frame, rgb_frame, cv::COLOR_BGR2RGBA);

    cv::flip(rgb_frame, flipped_rgb_frame, 1); //make the image behavior like a mirror, flip it horizontally
    
    // Return a raw pointer to the pixel array
    return flipped_rgb_frame.data; 
}

int get_preprocessed_frame(float* out_buffer, int target_size) {
    if (frame.empty() || out_buffer == nullptr) return 0;

    // 1. Convert original BGR frame to 3-channel RGB (AI models don't want Alpha)
    cv::Mat rgb_ai;
    cv::cvtColor(frame, rgb_ai, cv::COLOR_BGR2RGB);

    // 2. Calculate scaling factor to fit inside target_size x target_size
    int orig_w = frame.cols;
    int orig_h = frame.rows;
    float scale = std::min((float)target_size / orig_w, (float)target_size / orig_h);
    
    int new_w = (int)(orig_w * scale);
    int new_h = (int)(orig_h * scale);

    // 1. Shrink FIRST (much faster than converting color on 1280x720)
    cv::resize(frame, resized_bgr, cv::Size(new_w, new_h), 0, 0, cv::INTER_NEAREST);

    // 3. Pad with black bars (letterboxing) to make a perfect target_size x target_size square
    // 2. Pad the small image
    int pad_top = (target_size - new_h) / 2;
    int pad_bottom = target_size - new_h - pad_top;
    int pad_left = (target_size - new_w) / 2;
    int pad_right = target_size - new_w - pad_left;

    cv::copyMakeBorder(resized_bgr, padded_bgr, pad_top, pad_bottom, pad_left, pad_right, 
                       cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));

    // 4. Convert uint8 [0-255] to float32 [0.0-1.0] directly into a wrapper Mat
    // 3. Convert BGR -> RGB only on the tiny 192x192 image (36,864 pixels instead of 921,600!)
    cv::cvtColor(padded_bgr, padded_rgb, cv::COLOR_BGR2RGB);

    // 4. Normalize to float32 [0.0, 1.0]
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