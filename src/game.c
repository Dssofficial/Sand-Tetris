#include "game.h"
#include "config.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>
#include <stdbool.h>
#include <stdio.h>

#define unpack_color(color) (color.r), (color.g), (color.b), (color.a)

const struct Tetromino I_BLOCK = {
        .shape = {
                { // Rotation 1: rotation left of rotation 4
                        {0, 0, 0, 0},
                        {1, 1, 1, 1},
                        {0, 0, 0, 0},
                        {0, 0, 0, 0},
                },
                { // Rotation 2: rotation left of rotation 1
                        {0, 1, 0, 0},
                        {0, 1, 0, 0},
                        {0, 1, 0, 0},
                        {0, 1, 0, 0},
                },
                { // Rotation 3: rotation left of rotation 2
                        {0, 0, 0, 0},
                        {0, 0, 0, 0},
                        {1, 1, 1, 1},
                        {0, 0, 0, 0},
                },
                { // Rotation 4: rotation left of rotation 3
                        {0, 0, 1, 0},
                        {0, 0, 1, 0},
                        {0, 0, 1, 0},
                        {0, 0, 1, 0},
                }
        }
};

bool game_init(GameContext* GC) {
        if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
                fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
                return false;
        }

        SDL_Window* window = SDL_CreateWindow(
                "2P Sand Tetris",
                SDL_WINDOWPOS_CENTERED,
                SDL_WINDOWPOS_CENTERED,
                WINDOW_WIDTH,
                WINDOW_HEIGHT,
                SDL_WINDOW_RESIZABLE
        );

        if (!window) {
                fprintf(stderr, "Window error: %s\n", SDL_GetError());
                SDL_Quit();
                return false;
        }

        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
        SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) {
                fprintf(stderr, "Renderer error: %s\n", SDL_GetError());
                SDL_DestroyWindow(window);
                SDL_Quit();
                return false;
        }

        //TODO: GC->gameData
        GC->renderer = renderer;
        GC->window = window;
        GC->running = true;
        GC->redraw = true;
        GC->last_time = SDL_GetTicks();
        GC->delta_time = 0.0f;

        // Setting up virtual resolution
        SDL_RenderSetLogicalSize(GC->renderer, VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
        SDL_RenderSetIntegerScale(GC->renderer, SDL_TRUE);
        return true;
}

void game_handle_events(GameContext* GC) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
                switch (event.type) {
                        case SDL_QUIT:
                        GC->running = false;
                        break;

                        case SDL_KEYDOWN:
                        switch (event.key.keysym.sym) {
                                case SDLK_ESCAPE:
                                GC->running = false;
                                break;
                        }
                        break;
                }
        }
}

void game_update(GameContext* GC) {}

void game_render(GameContext* GC) {
        if (GC->redraw) {
                // 1. Clear to BLACK (this paints the bars)
                SDL_SetRenderDrawColor(GC->renderer, 0, 0, 0, 255);
                SDL_RenderClear(GC->renderer);

                // 2. Draw your game in RED (logical space)
                if (DEBUG) {
                        SDL_SetRenderDrawColor(GC->renderer, 255, 0, 0, 255);
                }
                SDL_Rect r = { 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT };
                SDL_RenderFillRect(GC->renderer, &r);

                // 3. Present the modified renderer
                SDL_RenderPresent(GC->renderer);
                GC->redraw = false;
        }
}

void game_cleanup(GameContext* GC) {
        SDL_DestroyRenderer(GC->renderer);
        SDL_DestroyWindow(GC->window);
        SDL_Quit();
}