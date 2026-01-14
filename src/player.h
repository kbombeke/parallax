#ifndef PLAYER_H
#define PLAYER_H

#include "map.h"

typedef struct {
    float x, y;      // Position in world coordinates
    float angle;     // Rotation angle in radians
    float move_speed;
    float rot_speed;
} Player;

// Initialize player at starting position
void player_init(Player* player);

// Update player movement based on input
void player_update(Player* player, const Map* map, float delta_time, 
                   int move_forward, int move_backward, 
                   int move_left, int move_right,
                   int rotate_left, int rotate_right,
                   float mouse_rotation_delta);

// Get player position
void player_get_position(const Player* player, float* x, float* y);

// Get player angle
float player_get_angle(const Player* player);

#endif // PLAYER_H
