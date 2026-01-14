#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "map.h"
#include "player.h"
#include "renderer.h"
#include "head_tracker.h"
#include "camera.h"

#define TARGET_FPS 60
#define FRAME_TIME (1.0f / TARGET_FPS)

static void render_quad(unsigned int texture_id, int screen_width, int screen_height) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, screen_width, screen_height, 0, -1, 1);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(0, 0);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(screen_width, 0);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(screen_width, screen_height);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(0, screen_height);
    glEnd();
    
    glDisable(GL_TEXTURE_2D);
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }
    
    // Set OpenGL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    
    // Create window
    int screen_width = SCREEN_WIDTH;
    int screen_height = SCREEN_HEIGHT;
    SDL_Window* window = SDL_CreateWindow(
        "Head-Tracked 3D Runner",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        screen_width,
        screen_height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
    );
    
    if (!window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    
    // Create OpenGL context
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        fprintf(stderr, "OpenGL context creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Enable relative mouse mode for mouse look
    SDL_SetRelativeMouseMode(SDL_TRUE);
    
    // Initialize systems
    Map map;
    map_init(&map);
    
    Player player;
    player_init(&player);
    
    Renderer* renderer = renderer_init(SCREEN_WIDTH, SCREEN_HEIGHT);
    if (!renderer) {
        fprintf(stderr, "Renderer initialization failed\n");
        SDL_GL_DeleteContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    HeadTracker* head_tracker = head_tracker_init(0);
    if (!head_tracker) {
        fprintf(stderr, "Warning: Head tracker initialization failed. Game will run without head tracking.\n");
    }
    
    Camera camera;
    camera_init(&camera);
    
    // Game state
    int running = 1;
    int move_forward = 0;
    int move_backward = 0;
    int move_left = 0;
    int move_right = 0;
    int rotate_left = 0;
    int rotate_right = 0;
    
    // Mouse sensitivity (rotation speed per pixel)
    #define MOUSE_SENSITIVITY 0.003f
    
    Uint32 last_time = SDL_GetTicks();
    
    printf("Head-Tracked 3D Runner\n");
    printf("Controls: Arrow keys to move, Mouse to look around, Q/E to rotate, ESC to exit\n");
    if (head_tracker) {
        printf("Head tracking: Active\n");
    } else {
        printf("Head tracking: Disabled (camera not available)\n");
    }
    
    // Main game loop
    while (running) {
        Uint32 current_time = SDL_GetTicks();
        float delta_time = (current_time - last_time) / 1000.0f;
        if (delta_time > 0.1f) delta_time = 0.1f; // Cap delta time
        last_time = current_time;
        
        // Handle events
        SDL_Event event;
        float mouse_rotation = 0.0f; // Accumulated mouse rotation for this frame
        
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = 0;
                    break;
                    
                case SDL_MOUSEMOTION:
                    // Use horizontal mouse movement to rotate
                    mouse_rotation += event.motion.xrel * MOUSE_SENSITIVITY;
                    break;
                    
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                        case SDLK_ESCAPE:
                            running = 0;
                            break;
                        case SDLK_UP:
                            move_forward = 1;
                            break;
                        case SDLK_DOWN:
                            move_backward = 1;
                            break;
                        case SDLK_LEFT:
                            move_left = 1;
                            break;
                        case SDLK_RIGHT:
                            move_right = 1;
                            break;
                        case SDLK_q:
                            rotate_left = 1;
                            break;
                        case SDLK_e:
                            rotate_right = 1;
                            break;
                    }
                    break;
                    
                case SDL_KEYUP:
                    switch (event.key.keysym.sym) {
                        case SDLK_UP:
                            move_forward = 0;
                            break;
                        case SDLK_DOWN:
                            move_backward = 0;
                            break;
                        case SDLK_LEFT:
                            move_left = 0;
                            break;
                        case SDLK_RIGHT:
                            move_right = 0;
                            break;
                        case SDLK_q:
                            rotate_left = 0;
                            break;
                        case SDLK_e:
                            rotate_right = 0;
                            break;
                    }
                    break;
            }
        }
        
        // Update player
        player_update(&player, &map, delta_time, move_forward, move_backward, move_left, move_right, rotate_left, rotate_right, mouse_rotation);
        
        // Update head tracking only when player is stopped (OpenCV face detection is expensive)
        if (head_tracker && !move_forward && !move_backward && !move_left && !move_right) {
            HeadPosition head_pos = head_tracker_update(head_tracker);
            camera_update(&camera, &head_pos);
        } else {
            // Reset camera offset when moving (head tracking disabled for performance)
            HeadPosition no_pos = {0.0f, 0.0f, 0.0f, 0};
            camera_update(&camera, &no_pos);
        }
        
        // Render
        renderer_render(renderer, &map, &player, &camera);
        
        // Clear screen
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Draw rendered texture
        unsigned int texture_id = renderer_get_texture(renderer);
        render_quad(texture_id, screen_width, screen_height);
        
        // Swap buffers
        SDL_GL_SwapWindow(window);
        
        // Frame rate limiting
        Uint32 frame_time = SDL_GetTicks() - current_time;
        if (frame_time < FRAME_TIME * 1000) {
            SDL_Delay((Uint32)(FRAME_TIME * 1000) - frame_time);
        }
    }
    
    // Cleanup
    if (head_tracker) {
        head_tracker_destroy(head_tracker);
    }
    renderer_destroy(renderer);
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}
