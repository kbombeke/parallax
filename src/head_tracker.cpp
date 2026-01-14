#include "head_tracker.h"
#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/imgproc.hpp>
#include <stdlib.h>
#include <string.h>

using namespace cv;

// C++ wrapper for OpenCV (OpenCV doesn't have a pure C API)
extern "C" {
    

#define SMOOTHING_ALPHA 0.7f  // Higher alpha = more smoothing (less jitter)
#define CALIBRATION_FRAMES 30  // Number of frames to calibrate center position
#define DEAD_ZONE 0.01f  // Dead zone around center to prevent drift (normalized units)
#define DRIFT_DECAY 0.98f  // Decay factor when in dead zone to return to zero
#define FACE_DETECT_SKIP_FRAMES 3  // Only do expensive face detection every N frames

struct HeadTracker {
    VideoCapture* cap;
    CascadeClassifier* face_cascade;
    float smoothed_x, smoothed_y, smoothed_z;
    int frame_count;
    int valid_frames;
    float calibrated_center_x;  // Calibrated center X position (normalized)
    int calibration_done;         // Whether calibration is complete
    int calibration_frame_count; // Frames used for calibration
    int last_detect_frame;      // Last frame where face detection was performed
    float last_raw_x;           // Last detected raw X position (cached)
};

HeadTracker* head_tracker_init(int camera_index) {
    HeadTracker* tracker = (HeadTracker*)malloc(sizeof(HeadTracker));
    if (!tracker) return NULL;
    
    memset(tracker, 0, sizeof(HeadTracker));
    
    // Initialize video capture
    tracker->cap = new VideoCapture(camera_index);
    if (!tracker->cap->isOpened()) {
        free(tracker);
        return NULL;
    }
    
    // Set camera resolution (lower for better performance)
    tracker->cap->set(CAP_PROP_FRAME_WIDTH, 640);
    tracker->cap->set(CAP_PROP_FRAME_HEIGHT, 480);
    
    // Load face detection cascade
    tracker->face_cascade = new CascadeClassifier();
    
    // Try to load face cascade from common OpenCV paths
    const char* cascade_paths[] = {
        "/usr/local/share/opencv4/haarcascades/haarcascade_frontalface_alt.xml",
        "/usr/share/opencv4/haarcascades/haarcascade_frontalface_alt.xml",
        "/opt/homebrew/share/opencv4/haarcascades/haarcascade_frontalface_alt.xml",
        "haarcascade_frontalface_alt.xml"
    };
    
    int loaded = 0;
    for (int i = 0; i < 4; i++) {
        if (tracker->face_cascade->load(cascade_paths[i])) {
            loaded = 1;
            break;
        }
    }
    
    if (!loaded) {
        // Try additional common paths
        const char* additional_paths[] = {
            "/usr/local/share/opencv/haarcascades/haarcascade_frontalface_alt.xml",
            "/usr/share/opencv/haarcascades/haarcascade_frontalface_alt.xml",
            "/opt/homebrew/share/opencv/haarcascades/haarcascade_frontalface_alt.xml"
        };
        
        for (int i = 0; i < 3; i++) {
            if (tracker->face_cascade->load(additional_paths[i])) {
                loaded = 1;
                break;
            }
        }
        
        if (!loaded) {
            delete tracker->face_cascade;
            delete tracker->cap;
            free(tracker);
            return NULL;
        }
    }
    
    tracker->smoothed_x = 0.0f;
    tracker->smoothed_y = 0.0f;
    tracker->smoothed_z = 0.0f;
    tracker->calibrated_center_x = 0.0f;
    tracker->calibration_done = 0;
    tracker->calibration_frame_count = 0;
    tracker->last_detect_frame = -1;
    tracker->last_raw_x = 0.0f;
    
    return tracker;
}

void head_tracker_destroy(HeadTracker* tracker) {
    if (!tracker) return;
    
    if (tracker->cap) {
        tracker->cap->release();
        delete tracker->cap;
    }
    if (tracker->face_cascade) {
        delete tracker->face_cascade;
    }
    free(tracker);
}

HeadPosition head_tracker_update(HeadTracker* tracker) {
    HeadPosition pos = {0.0f, 0.0f, 0.0f, 0};
    
    if (!tracker || !tracker->cap || !tracker->face_cascade) {
        return pos;
    }
    
    Mat frame;
    if (!tracker->cap->read(frame) || frame.empty()) {
        return pos;
    }
    
    tracker->frame_count++;
    
    // Performance optimization: only do expensive face detection every N frames
    // Use cached value in between frames
    float raw_x;
    int should_detect = (tracker->frame_count - tracker->last_detect_frame) >= FACE_DETECT_SKIP_FRAMES;
    
    if (should_detect) {
        // Convert to grayscale for face detection
        Mat gray;
        cvtColor(frame, gray, COLOR_BGR2GRAY);
        equalizeHist(gray, gray);
        
        // Detect faces (expensive operation)
        std::vector<Rect> faces;
        tracker->face_cascade->detectMultiScale(gray, faces, 1.1, 3, 0, Size(30, 30));
        
        if (faces.empty()) {
            // No face detected - gradually decay the smoothed position
            tracker->smoothed_x *= 0.95f;
            tracker->smoothed_y *= 0.95f;
            tracker->smoothed_z *= 0.95f;
            
            if (tracker->valid_frames > 0) {
                tracker->valid_frames--;
            }
            
            pos.x = tracker->smoothed_x;
            pos.y = tracker->smoothed_y;
            pos.z = tracker->smoothed_z;
            pos.valid = tracker->valid_frames > 5;
            tracker->last_detect_frame = tracker->frame_count;
            return pos;
        }
        
        // Use the largest face (closest to camera)
        Rect largest_face = faces[0];
        for (size_t i = 1; i < faces.size(); i++) {
            if (faces[i].area() > largest_face.area()) {
                largest_face = faces[i];
            }
        }
        
        // Calculate face center relative to frame center
        int frame_center_x = frame.cols / 2;
        int face_center_x = largest_face.x + largest_face.width / 2;
        
        // Normalize to [-1, 1] range
        raw_x = (float)(face_center_x - frame_center_x) / (float)frame_center_x;
        tracker->last_raw_x = raw_x;
        tracker->last_detect_frame = tracker->frame_count;
    } else {
        // Use cached value from last detection
        raw_x = tracker->last_raw_x;
    }
    
    // Calibration phase: accumulate center position
    if (!tracker->calibration_done) {
        tracker->calibrated_center_x += raw_x;
        tracker->calibration_frame_count++;
        
        if (tracker->calibration_frame_count >= CALIBRATION_FRAMES) {
            // Calibration complete - set center as average
            tracker->calibrated_center_x /= (float)CALIBRATION_FRAMES;
            tracker->calibration_done = 1;
        }
        
        // During calibration, return zero offset
        pos.x = 0.0f;
        pos.y = 0.0f;
        pos.z = 0.0f;
        pos.valid = 1;
        return pos;
    }
    
    // After calibration: calculate horizontal deviation from calibrated center
    float horizontal_deviation = raw_x - tracker->calibrated_center_x;
    
    // Apply dead zone to prevent drift when head is stationary
    if (fabsf(horizontal_deviation) < DEAD_ZONE) {
        horizontal_deviation = 0.0f;
    }
    
    // Apply smoothing to deviation
    tracker->smoothed_x = SMOOTHING_ALPHA * tracker->smoothed_x + (1.0f - SMOOTHING_ALPHA) * horizontal_deviation;
    
    // If smoothed value is in dead zone, decay it toward zero to prevent drift
    if (fabsf(tracker->smoothed_x) < DEAD_ZONE) {
        tracker->smoothed_x *= DRIFT_DECAY;
    }
    
    tracker->valid_frames = 30; // Valid for next 30 frames even if face lost
    
    // Return only horizontal deviation (exaggerated parallax will be applied in camera)
    pos.x = tracker->smoothed_x;
    pos.y = 0.0f;  // Ignore vertical
    pos.z = 0.0f;  // Ignore depth
    pos.valid = 1;
    
    return pos;
}

int head_tracker_is_valid(const HeadTracker* tracker) {
    return tracker && tracker->cap && tracker->cap->isOpened();
}
} // extern "C"
