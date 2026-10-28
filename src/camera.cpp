#include "camera.h"
#include <opencv2/opencv.hpp>

// Static instances hide the C++ objects from your C code
static cv::VideoCapture cap;
static cv::Mat frame;
static cv::Mat rgb_frame;
static cv::Mat flipped_rgb_frame;

extern "C" {

int init_camera(int width, int height) {
    // 1. Force the native Linux V4L2 backend (bypasses the buggy GStreamer)
    cap.open(0, cv::CAP_V4L2); 
    if (!cap.isOpened()) return 0;
    
    // 2. Force MJPEG compression to allow HD resolutions over USB
    cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
    
    // 3. Request your dimensions
    cap.set(cv::CAP_PROP_FRAME_WIDTH, width);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, height);
    
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

void close_camera(void) {
    if (cap.isOpened()) {
        cap.release();
    }
}

}