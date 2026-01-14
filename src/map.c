#include "map.h"
#include <string.h>

void map_init(Map* map) {
    // Initialize all cells as empty
    memset(map->grid, CELL_EMPTY, sizeof(map->grid));
    
    // Create outer walls
    for (int i = 0; i < MAP_WIDTH; i++) {
        map->grid[0][i] = CELL_WALL;
        map->grid[MAP_HEIGHT - 1][i] = CELL_WALL;
        map->grid[i][0] = CELL_WALL;
        map->grid[i][MAP_WIDTH - 1] = CELL_WALL;
    }
    
    // Add some interior walls to create a simple room layout
    // Vertical walls
    for (int i = 5; i < 15; i++) {
        map->grid[i][10] = CELL_WALL;
    }
    
    // Horizontal walls
    for (int i = 8; i < 18; i++) {
        map->grid[15][i] = CELL_WALL;
    }
    
    // Add some pillars/obstacles
    map->grid[5][5] = CELL_WALL;
    map->grid[18][18] = CELL_WALL;
    map->grid[5][18] = CELL_WALL;
    map->grid[18][5] = CELL_WALL;
    
    // Initialize objects
    map->object_count = 4;
    map->objects[0] = (Object){12, 12, OBJ_BOX};
    map->objects[1] = (Object){8, 8, OBJ_BOX};
    map->objects[2] = (Object){16, 8, OBJ_BOX};
    map->objects[3] = (Object){12, 16, OBJ_BOX};
}

int map_is_wall(const Map* map, int x, int y) {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) {
        return 1; // Out of bounds counts as wall
    }
    return map->grid[y][x] == CELL_WALL;
}

int map_check_collision(const Map* map, float x, float y, float radius) {
    // Check collision with walls using a small radius
    int map_x = (int)x;
    int map_y = (int)y;
    
    // Check current cell and adjacent cells
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (map_is_wall(map, map_x + dx, map_y + dy)) {
                // Check distance from wall
                float wall_x = map_x + dx;
                float wall_y = map_y + dy;
                float dist_x = x - wall_x;
                float dist_y = y - wall_y;
                float dist_sq = dist_x * dist_x + dist_y * dist_y;
                
                if (dist_sq < radius * radius) {
                    return 1;
                }
            }
        }
    }
    
    return 0;
}

const int* map_get_grid(const Map* map) {
    return (const int*)map->grid;
}
