#ifndef CAMERA_H
#define CAMERA_H

#ifdef __cplusplus
extern "C" {
#endif

int init_camera(int width, int height);
unsigned char* get_camera_frame(void);
void close_camera(void);

#ifdef __cplusplus
}
#endif

#endif