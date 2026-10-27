#include "camera.h"
#include <opencv2/opencv.hpp>

// Static instances hide the C++ objects from your C code
static cv::VideoCapture cap;
static cv::Mat frame;
static cv::Mat rgb_frame;

extern "C" {

int init_camera(int width, int height) {
    cap.open(0); // Open the default webcam (index 0)
    if (!cap.isOpened()) return 0;
    
    // Request a specific resolution
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
    
    // Return a raw pointer to the pixel array
    return rgb_frame.data; 
}

void close_camera(void) {
    if (cap.isOpened()) {
        cap.release();
    }
}

}