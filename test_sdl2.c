#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#include <SDL2/SDL.h>
// Normally SDL2 will redefine the main entry point of the program for Windows applications
// this doesn't seem to play nice with TCC, so we just undefine the redefinition
#ifdef __TINYC__
    #undef main
#endif

// Utility macros
#define CHECK_ERROR(test, message) \
    do { \
        if((test)) { \
            fprintf(stderr, "%s\n", (message)); \
            exit(1); \
        } \
    } while(0)

// Get a random number from 0 to 255
int randInt(int rmin, int rmax) {
    return rand() % rmax + rmin;
}
    
// Window dimensions
static const int width = 800;
static const int height = 600;

int main(int argc, char **argv) {
    // Initialize the random number generator
    srand(time(NULL));
    
    // Initialize SDL
    CHECK_ERROR(SDL_Init(SDL_INIT_VIDEO) != 0, SDL_GetError());

    // Create an SDL window
    SDL_Window *window = SDL_CreateWindow("Hello, SDL2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_OPENGL);
    CHECK_ERROR(window == NULL, SDL_GetError());

    // Create a renderer (accelerated and in sync with the display refresh rate)
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);    
    CHECK_ERROR(renderer == NULL, SDL_GetError());

    // Initial renderer color
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

    bool running = true;
    SDL_Event event;
    while(running) {
        // Process events
        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_QUIT) {
                running = false;
            } else if(event.type == SDL_KEYDOWN) {
                const char *key = SDL_GetKeyName(event.key.keysym.sym);
                if(strcmp(key, "C") == 0) {
                    SDL_SetRenderDrawColor(renderer, randInt(0, 255), randInt(0, 255), randInt(0, 255), 255);
                } else if(strcmp(key, "Q") == 0) {
                    running = false;
                }                    
            }
        }

        // Clear screen
        SDL_RenderClear(renderer);

        // Draw

        // Show what was drawn
        SDL_RenderPresent(renderer);
    }

    // Release resources
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

