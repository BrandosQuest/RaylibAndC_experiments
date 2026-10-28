#ifndef CAMERA_H
#define CAMERA_H

#ifdef __cplusplus
extern "C" {
#endif

int init_camera(int width, int height);
unsigned char* get_camera_frame(void);

// Fills out_buffer with normalized [0.0f - 1.0f] RGB floats resized/padded to target_size x target_size
int get_preprocessed_frame(float* out_buffer, int target_size);

void close_camera(void);

#ifdef __cplusplus
}
#endif

#endif