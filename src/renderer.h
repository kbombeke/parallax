#ifndef RENDERER_H
#define RENDERER_H

#include "map.h"
#include "player.h"
#include "camera.h"

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768

typedef struct Renderer Renderer;

// Initialize renderer (creates OpenGL context and texture)
Renderer* renderer_init(int width, int height);

// Cleanup renderer
void renderer_destroy(Renderer* renderer);

// Render frame using raycasting
void renderer_render(Renderer* renderer, const Map* map, const Player* player, const Camera* camera);

// Get OpenGL texture ID for the rendered frame
unsigned int renderer_get_texture(const Renderer* renderer);

// Get renderer dimensions
void renderer_get_size(const Renderer* renderer, int* width, int* height);

#endif // RENDERER_H
