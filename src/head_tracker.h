#ifndef HEAD_TRACKER_H
#define HEAD_TRACKER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float x, y, z;  // Head position offset relative to screen center (normalized)
    int valid;      // Whether tracking is currently valid
} HeadPosition;

typedef struct HeadTracker HeadTracker;

// Initialize head tracker (opens webcam)
HeadTracker* head_tracker_init(int camera_index);

// Cleanup head tracker
void head_tracker_destroy(HeadTracker* tracker);

// Update head tracking (call each frame)
HeadPosition head_tracker_update(HeadTracker* tracker);

// Check if tracker is initialized and working
int head_tracker_is_valid(const HeadTracker* tracker);

#ifdef __cplusplus
}
#endif

#endif // HEAD_TRACKER_H
