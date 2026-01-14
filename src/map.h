#ifndef MAP_H
#define MAP_H

#define MAP_WIDTH 24
#define MAP_HEIGHT 24

// Map cell types
#define CELL_EMPTY 0
#define CELL_WALL 1

// Object types
#define OBJ_BOX 1

typedef struct {
    int x, y;
    int type;
} Object;

typedef struct {
    int grid[MAP_WIDTH][MAP_HEIGHT];
    Object objects[32];
    int object_count;
} Map;

// Initialize the map with a simple room layout
void map_init(Map* map);

// Check if a position is a wall
int map_is_wall(const Map* map, int x, int y);

// Check if a position collides with any wall (for float coordinates)
int map_check_collision(const Map* map, float x, float y, float radius);

// Get the map grid
const int* map_get_grid(const Map* map);

#endif // MAP_H
