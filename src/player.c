#include "player.h"
#include <math.h>

#define PLAYER_START_X 2.0f
#define PLAYER_START_Y 2.0f
#define PLAYER_START_ANGLE 0.0f
#define PLAYER_MOVE_SPEED 3.0f
#define PLAYER_ROT_SPEED 2.0f
#define PLAYER_RADIUS 0.2f

void player_init(Player* player) {
    player->x = PLAYER_START_X;
    player->y = PLAYER_START_Y;
    player->angle = PLAYER_START_ANGLE;
    player->move_speed = PLAYER_MOVE_SPEED;
    player->rot_speed = PLAYER_ROT_SPEED;
}

void player_update(Player* player, const Map* map, float delta_time,
                   int move_forward, int move_backward,
                   int move_left, int move_right,
                   int rotate_left, int rotate_right,
                   float mouse_rotation_delta) {
    // Handle rotation
    // Mouse rotation takes priority (more responsive)
    if (fabsf(mouse_rotation_delta) > 0.0001f) {
        player->angle += mouse_rotation_delta;
    } else {
        // Fall back to keyboard rotation
        if (rotate_left) {
            player->angle -= player->rot_speed * delta_time;
        }
        if (rotate_right) {
            player->angle += player->rot_speed * delta_time;
        }
    }
    
    // Normalize angle to [0, 2*PI)
    while (player->angle < 0.0f) player->angle += 2.0f * M_PI;
    while (player->angle >= 2.0f * M_PI) player->angle -= 2.0f * M_PI;
    
    float move_x = 0.0f;
    float move_y = 0.0f;
    
    // Calculate movement direction based on player angle
    if (move_forward) {
        move_x += cosf(player->angle) * player->move_speed * delta_time;
        move_y += sinf(player->angle) * player->move_speed * delta_time;
    }
    if (move_backward) {
        move_x -= cosf(player->angle) * player->move_speed * delta_time;
        move_y -= sinf(player->angle) * player->move_speed * delta_time;
    }
    if (move_left) {
        move_x += cosf(player->angle - M_PI / 2.0f) * player->move_speed * delta_time;
        move_y += sinf(player->angle - M_PI / 2.0f) * player->move_speed * delta_time;
    }
    if (move_right) {
        move_x += cosf(player->angle + M_PI / 2.0f) * player->move_speed * delta_time;
        move_y += sinf(player->angle + M_PI / 2.0f) * player->move_speed * delta_time;
    }
    
    // Try to move in X direction first
    float new_x = player->x + move_x;
    float new_y = player->y;
    if (!map_check_collision(map, new_x, new_y, PLAYER_RADIUS)) {
        player->x = new_x;
    }
    
    // Then try to move in Y direction
    new_x = player->x;
    new_y = player->y + move_y;
    if (!map_check_collision(map, new_x, new_y, PLAYER_RADIUS)) {
        player->y = new_y;
    }
}

void player_get_position(const Player* player, float* x, float* y) {
    *x = player->x;
    *y = player->y;
}

float player_get_angle(const Player* player) {
    return player->angle;
}
