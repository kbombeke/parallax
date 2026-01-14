#include "renderer.h"
#include <SDL2/SDL_opengl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

struct Renderer {
    int width, height;
    unsigned int texture_id;
    unsigned char* framebuffer;
    float* depth_buffer;
};

Renderer* renderer_init(int width, int height) {
    Renderer* renderer = (Renderer*)malloc(sizeof(Renderer));
    if (!renderer) return NULL;
    
    renderer->width = width;
    renderer->height = height;
    renderer->framebuffer = (unsigned char*)malloc(width * height * 3);
    renderer->depth_buffer = (float*)malloc(width * sizeof(float));
    
    if (!renderer->framebuffer || !renderer->depth_buffer) {
        free(renderer->framebuffer);
        free(renderer->depth_buffer);
        free(renderer);
        return NULL;
    }
    
    // Create OpenGL texture
    glGenTextures(1, &renderer->texture_id);
    glBindTexture(GL_TEXTURE_2D, renderer->texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    return renderer;
}

void renderer_destroy(Renderer* renderer) {
    if (!renderer) return;
    
    if (renderer->texture_id) {
        glDeleteTextures(1, &renderer->texture_id);
    }
    free(renderer->framebuffer);
    free(renderer->depth_buffer);
    free(renderer);
}

// DDA algorithm for raycasting - returns distance and wall hit position
static float cast_ray(const Map* map, float start_x, float start_y, 
                      float dir_x, float dir_y, int* side, float* wall_x, int* map_x_hit, int* map_y_hit) {
    // Safety check: avoid division by zero
    if (fabsf(dir_x) < 0.0001f && fabsf(dir_y) < 0.0001f) {
        // Invalid direction, return large distance
        *side = 0;
        *wall_x = 0.0f;
        *map_x_hit = (int)start_x;
        *map_y_hit = (int)start_y;
        return 1000.0f;
    }
    
    int map_x = (int)start_x;
    int map_y = (int)start_y;
    
    float delta_dist_x = fabsf(dir_x) < 0.0001f ? 1e10f : fabsf(1.0f / dir_x);
    float delta_dist_y = fabsf(dir_y) < 0.0001f ? 1e10f : fabsf(1.0f / dir_y);
    
    int step_x, step_y;
    float side_dist_x, side_dist_y;
    
    if (dir_x < 0) {
        step_x = -1;
        side_dist_x = (start_x - map_x) * delta_dist_x;
    } else {
        step_x = 1;
        side_dist_x = (map_x + 1.0f - start_x) * delta_dist_x;
    }
    
    if (dir_y < 0) {
        step_y = -1;
        side_dist_y = (start_y - map_y) * delta_dist_y;
    } else {
        step_y = 1;
        side_dist_y = (map_y + 1.0f - start_y) * delta_dist_y;
    }
    
    int hit = 0;
    while (!hit) {
        if (side_dist_x < side_dist_y) {
            side_dist_x += delta_dist_x;
            map_x += step_x;
            *side = 0;
        } else {
            side_dist_y += delta_dist_y;
            map_y += step_y;
            *side = 1;
        }
        
        if (map_is_wall(map, map_x, map_y)) {
            hit = 1;
        }
    }
    
    float perp_wall_dist;
    float wall_hit_x, wall_hit_y;
    if (*side == 0) {
        if (fabsf(dir_x) < 0.0001f) {
            perp_wall_dist = 1000.0f; // Avoid division by zero
        } else {
            perp_wall_dist = (map_x - start_x + (1 - step_x) / 2) / dir_x;
        }
        wall_hit_x = (float)map_x + (step_x < 0 ? 1.0f : 0.0f);
        wall_hit_y = start_y + perp_wall_dist * dir_y;
    } else {
        if (fabsf(dir_y) < 0.0001f) {
            perp_wall_dist = 1000.0f; // Avoid division by zero
        } else {
            perp_wall_dist = (map_y - start_y + (1 - step_y) / 2) / dir_y;
        }
        wall_hit_x = start_x + perp_wall_dist * dir_x;
        wall_hit_y = (float)map_y + (step_y < 0 ? 1.0f : 0.0f);
    }
    
    // Safety check: ensure distance is positive and reasonable
    if (perp_wall_dist < 0.01f) perp_wall_dist = 0.01f;
    if (perp_wall_dist > 1000.0f) perp_wall_dist = 1000.0f;
    
    // Store hit position
    *map_x_hit = map_x;
    *map_y_hit = map_y;
    
    // Normalize wall_x to [0, 1] range within the wall cell for texture mapping
    if (*side == 0) {
        *wall_x = wall_hit_y - floorf(wall_hit_y);
    } else {
        *wall_x = wall_hit_x - floorf(wall_hit_x);
    }
    
    return perp_wall_dist;
}

void renderer_render(Renderer* renderer, const Map* map, const Player* player, const Camera* camera) {
    if (!renderer || !map || !player || !camera) return;
    
    // Clear framebuffer (black background)
    memset(renderer->framebuffer, 0, renderer->width * renderer->height * 3);
    
    float player_x, player_y;
    player_get_position(player, &player_x, &player_y);
    float player_angle = player_get_angle(player);
    
    // Get camera offset for parallax
    float cam_offset_x, cam_offset_y;
    camera_get_offset(camera, &cam_offset_x, &cam_offset_y);
    
    // Clamp camera offset to prevent extreme values that could cause crashes
    // Maximum offset should be reasonable relative to map cell size (1.0)
    if (cam_offset_x > 0.5f) cam_offset_x = 0.5f;
    if (cam_offset_x < -0.5f) cam_offset_x = -0.5f;
    if (cam_offset_y > 0.5f) cam_offset_y = 0.5f;
    if (cam_offset_y < -0.5f) cam_offset_y = -0.5f;
    
    // Camera plane (for FOV)
    float fov = camera_get_fov(camera);
    float plane_x = -sinf(player_angle) * tanf(fov / 2.0f);
    float plane_y = cosf(player_angle) * tanf(fov / 2.0f);
    
    // Direction vector
    float dir_x = cosf(player_angle);
    float dir_y = sinf(player_angle);

    // Convert camera offset (in "screen space") into a lateral world-space offset.
    // We move the virtual camera sideways along the camera plane (perpendicular to view direction),
    // so head movement left/right produces pure horizontal parallax (no forward/back motion).
    float offset_world_x = plane_x * cam_offset_x;
    float offset_world_y = plane_y * cam_offset_x;
    
    // Cast rays for each column
    for (int x = 0; x < renderer->width; x++) {
        // Calculate ray position and direction
        float camera_x = 2.0f * x / (float)renderer->width - 1.0f; // -1 to 1
        float ray_dir_x = dir_x + plane_x * camera_x;
        float ray_dir_y = dir_y + plane_y * camera_x;
        
        // Safety check: ensure ray direction is not degenerate
        float ray_dir_len_sq = ray_dir_x * ray_dir_x + ray_dir_y * ray_dir_y;
        if (ray_dir_len_sq < 0.0001f) {
            // Skip this column if direction is too small
            continue;
        }
        
        // Apply head tracking offset to ray origin (Window Mode)
        // Use world-space lateral offset so this only shifts the viewpoint sideways.
        float ray_start_x = player_x + offset_world_x;
        float ray_start_y = player_y + offset_world_y;
        
        // Cast ray
        int side;
        float wall_x;
        int map_x_hit, map_y_hit;
        float perp_wall_dist = cast_ray(map, ray_start_x, ray_start_y, ray_dir_x, ray_dir_y, &side, &wall_x, &map_x_hit, &map_y_hit);
        
        // Calculate height of line to draw on screen
        // Safety check: avoid division by zero or very small distances
        if (perp_wall_dist < 0.01f) perp_wall_dist = 0.01f;
        int line_height = (int)(renderer->height / perp_wall_dist);
        if (line_height > renderer->height) line_height = renderer->height;
        
        // Calculate lowest and highest pixel to fill
        int draw_start = -line_height / 2 + renderer->height / 2;
        if (draw_start < 0) draw_start = 0;
        int draw_end = line_height / 2 + renderer->height / 2;
        if (draw_end >= renderer->height) draw_end = renderer->height - 1;
        
        // Brick pattern parameters
        #define BRICK_WIDTH 0.25f   // Width of one brick (normalized 0-1)
        #define BRICK_HEIGHT 0.2f   // Height of one brick (normalized 0-1)
        #define MORTAR_WIDTH 0.03f  // Width of mortar lines (normalized 0-1)
        
        // Calculate horizontal texture coordinate (wall_x is already [0, 1])
        float tex_u = wall_x;
        
        // Draw the vertical line with brick pattern
        for (int y = draw_start; y <= draw_end; y++) {
            // Calculate vertical texture coordinate
            // Use screen position to create vertical pattern
            float screen_ratio = (float)(y - draw_start) / (float)(draw_end - draw_start + 1);
            float tex_v = screen_ratio; // Simple vertical coordinate
            
            // Scale to brick space
            float brick_u = tex_u / BRICK_WIDTH;
            float brick_v = tex_v / BRICK_HEIGHT;
            
            // Create staggered brick pattern (running bond)
            // Every other row is offset by half a brick
            int brick_row = (int)floorf(brick_v);
            float row_offset = (brick_row % 2 == 0) ? 0.0f : 0.5f;
            float brick_col = floorf(brick_u + row_offset);
            
            // Get position within current brick (normalized 0-1)
            float local_u = (brick_u + row_offset) - brick_col;
            float local_v = brick_v - brick_row;
            
            // Check if we're in a mortar line
            int is_mortar = 0;
            if (local_u < MORTAR_WIDTH || local_u > (1.0f - MORTAR_WIDTH) ||
                local_v < MORTAR_WIDTH || local_v > (1.0f - MORTAR_WIDTH)) {
                is_mortar = 1;
            }
            
            // Choose base color
            unsigned char base_color;
            if (is_mortar) {
                // Mortar is darker grey
                base_color = 60;
            } else {
                // Brick color with variation for texture
                float brick_variation = sinf(brick_col * 0.7f + brick_row * 0.5f) * 0.15f + 1.0f;
                if (side == 0) {
                    base_color = (unsigned char)(190 * brick_variation); // Lighter for east/west walls
                } else {
                    base_color = (unsigned char)(140 * brick_variation); // Darker for north/south walls
                }
            }
            
            // Apply distance shading (farther = darker)
            float shade = 1.0f / (1.0f + perp_wall_dist * 0.1f);
            if (shade > 1.0f) shade = 1.0f;
            unsigned char color = (unsigned char)(base_color * shade);
            
            int idx = (y * renderer->width + x) * 3;
            renderer->framebuffer[idx] = color;     // R
            renderer->framebuffer[idx + 1] = color; // G
            renderer->framebuffer[idx + 2] = color; // B
        }
        
        // Store depth for occlusion checking
        renderer->depth_buffer[x] = perp_wall_dist;
        
        // Draw floor (simple gradient)
        for (int y = draw_end + 1; y < renderer->height; y++) {
            int idx = (y * renderer->width + x) * 3;
            // Safety check: avoid division by zero when y == height/2
            float denominator = 2.0f * y - renderer->height;
            float floor_dist;
            if (fabsf(denominator) < 0.0001f) {
                floor_dist = 1000.0f; // Very far away
            } else {
                floor_dist = renderer->height / denominator;
            }
            float floor_shade = 1.0f / (1.0f + floor_dist * 0.05f);
            if (floor_shade > 1.0f) floor_shade = 1.0f;
            unsigned char floor_color = (unsigned char)(80 * floor_shade);
            renderer->framebuffer[idx] = floor_color;
            renderer->framebuffer[idx + 1] = floor_color;
            renderer->framebuffer[idx + 2] = floor_color;
        }
        
        // Draw ceiling (darker)
        for (int y = 0; y < draw_start; y++) {
            int idx = (y * renderer->width + x) * 3;
            unsigned char ceiling_color = 40;
            renderer->framebuffer[idx] = ceiling_color;
            renderer->framebuffer[idx + 1] = ceiling_color;
            renderer->framebuffer[idx + 2] = ceiling_color;
        }
    }
    
    // Render objects with occlusion (optimized - pre-filter visible objects)
    // First, find objects that are potentially visible (in front of player)
    int visible_objects[32];
    int visible_count = 0;
    
    for (int obj_idx = 0; obj_idx < map->object_count; obj_idx++) {
        const Object* obj = &map->objects[obj_idx];
        float obj_x = (float)obj->x + 0.5f;
        float obj_y = (float)obj->y + 0.5f;
        
        // Object position relative to player (with head tracking parallax).
        // Use the same lateral world offset used for the rays so occlusion matches.
        float obj_delta_x = obj_x - (player_x + offset_world_x);
        float obj_delta_y = obj_y - (player_y + offset_world_y);
        
        // Quick check: is object in front of player?
        float dot = obj_delta_x * dir_x + obj_delta_y * dir_y;
        if (dot <= 0.1f) continue; // Behind player
        
        // Check if object is within reasonable distance (not too far)
        float dist_sq = obj_delta_x * obj_delta_x + obj_delta_y * obj_delta_y;
        if (dist_sq > 100.0f) continue; // Too far away
        
        visible_objects[visible_count++] = obj_idx;
    }
    
    // Only process columns if there are visible objects
    if (visible_count > 0) {
        for (int x = 0; x < renderer->width; x++) {
            float wall_dist = renderer->depth_buffer[x];
            float closest_obj_dist = wall_dist;
            int closest_obj_idx = -1;
            
            // Calculate ray direction for this column (same as used for walls)
            float camera_x = 2.0f * x / (float)renderer->width - 1.0f;
            float ray_dir_x = dir_x + plane_x * camera_x;
            float ray_dir_y = dir_y + plane_y * camera_x;
            
            // Check only visible objects
            for (int v = 0; v < visible_count; v++) {
                int obj_idx = visible_objects[v];
                const Object* obj = &map->objects[obj_idx];
                float obj_x = (float)obj->x + 0.5f;
                float obj_y = (float)obj->y + 0.5f;
                
                // Object position relative to player (with head tracking parallax)
                float obj_delta_x = obj_x - (player_x + offset_world_x);
                float obj_delta_y = obj_y - (player_y + offset_world_y);
                
                // Check if object is behind player (re-check with ray direction)
                float dot = obj_delta_x * ray_dir_x + obj_delta_y * ray_dir_y;
                if (dot <= 0.1f) continue;
                
                // Calculate perpendicular distance from ray to object center
                float cross = obj_delta_x * ray_dir_y - obj_delta_y * ray_dir_x;
                float perp_dist = fabsf(cross);
                
                // Object size (radius) - check if ray passes through object
                float obj_radius = 0.4f;
                if (perp_dist > obj_radius) continue;
                
                // Distance to object along ray
                float obj_dist = dot;
                
                // Only consider if closer than wall and current closest object
                if (obj_dist < closest_obj_dist && obj_dist > 0.1f) {
                    closest_obj_dist = obj_dist;
                    closest_obj_idx = obj_idx;
                }
            }
            
            // Render closest object if it occludes the wall
            if (closest_obj_idx >= 0 && closest_obj_dist < wall_dist) {
                // Calculate object height on screen
                // Safety check: avoid division by zero
                if (closest_obj_dist < 0.01f) closest_obj_dist = 0.01f;
                int obj_line_height = (int)(renderer->height / closest_obj_dist);
                if (obj_line_height > renderer->height) obj_line_height = renderer->height;
                
                int obj_draw_start = -obj_line_height / 2 + renderer->height / 2;
                if (obj_draw_start < 0) obj_draw_start = 0;
                int obj_draw_end = obj_line_height / 2 + renderer->height / 2;
                if (obj_draw_end >= renderer->height) obj_draw_end = renderer->height - 1;
                
                // Object color (grey, darker than walls)
                float obj_shade = 1.0f / (1.0f + closest_obj_dist * 0.1f);
                if (obj_shade > 1.0f) obj_shade = 1.0f;
                unsigned char obj_color = (unsigned char)(120 * obj_shade);
                
                // Draw object column (occludes wall)
                for (int y = obj_draw_start; y <= obj_draw_end; y++) {
                    int idx = (y * renderer->width + x) * 3;
                    renderer->framebuffer[idx] = obj_color;
                    renderer->framebuffer[idx + 1] = obj_color;
                    renderer->framebuffer[idx + 2] = obj_color;
                }
            }
        }
    }
    
    // Update OpenGL texture
    glBindTexture(GL_TEXTURE_2D, renderer->texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, renderer->width, renderer->height, 
                 0, GL_RGB, GL_UNSIGNED_BYTE, renderer->framebuffer);
}

unsigned int renderer_get_texture(const Renderer* renderer) {
    return renderer ? renderer->texture_id : 0;
}

void renderer_get_size(const Renderer* renderer, int* width, int* height) {
    if (renderer) {
        *width = renderer->width;
        *height = renderer->height;
    }
}
