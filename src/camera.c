#include "camera.h"
#include <math.h>

#define DEFAULT_FOV (M_PI / 3.0f)  // 60 degrees
#define PARALLAX_SCALE 3.0f         // Exaggerated scale for horizontal parallax (small movements = big effect)
#define DEFAULT_NEAR 0.1f
#define DEFAULT_FAR 100.0f

void camera_init(Camera* camera) {
    camera->offset_x = 0.0f;
    camera->offset_y = 0.0f;
    camera->fov = DEFAULT_FOV;
    camera->near_plane = DEFAULT_NEAR;
    camera->far_plane = DEFAULT_FAR;
}

void camera_update(Camera* camera, const HeadPosition* head_pos) {
    if (!head_pos || !head_pos->valid) {
        // Gradually decay offset when tracking is lost
        camera->offset_x *= 0.95f;
        camera->offset_y = 0.0f;  // Always keep vertical offset zero (no vertical parallax)
        return;
    }
    
    // Exaggerated horizontal parallax: small head movements create large parallax effect
    // In real life: when head moves RIGHT, you see more of the LEFT side (inverted)
    // So we negate the offset: head right (positive) -> view shifts left (negative offset)
    // This allows "looking around corners" with small head movements
    // Only horizontal parallax is used (vertical ignored)
    
    camera->offset_x = -head_pos->x * PARALLAX_SCALE;  // Inverted for realistic parallax
    camera->offset_y = 0.0f;  // No vertical parallax
}

void camera_get_offset(const Camera* camera, float* offset_x, float* offset_y) {
    *offset_x = camera->offset_x;
    *offset_y = camera->offset_y;
}

float camera_get_fov(const Camera* camera) {
    return camera->fov;
}
