#ifndef CAMERA_H
#define CAMERA_H

#include "head_tracker.h"

typedef struct {
    float offset_x, offset_y;  // Camera offset for parallax effect
    float fov;                 // Field of view
    float near_plane;
    float far_plane;
} Camera;

// Initialize camera with default settings
void camera_init(Camera* camera);

// Update camera based on head position (Window Mode)
void camera_update(Camera* camera, const HeadPosition* head_pos);

// Get camera offset for raycasting
void camera_get_offset(const Camera* camera, float* offset_x, float* offset_y);

// Get field of view
float camera_get_fov(const Camera* camera);

#endif // CAMERA_H
