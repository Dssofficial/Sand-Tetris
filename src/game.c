#include "game.h"
#include "config.h"
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_blendmode.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define unpack_color(color) (color.r), (color.g), (color.b), (color.a)
#define randColor() (rand() % COLOR_COUNT)
#define randRotation() (rand() % 4) // 4 rotations total so MAGIC NUMBER

// One time Function
static inline void InitializeTetriminoCollection(TetrominoCollection* TC);
static inline void CleanUpTetriminoCollection(TetrominoCollection* TC);

// This function called to create new tetrimino
// For currentTetrimino once in init
// For every nextTetrimino determination
static void InitializeTetriminoData(TetrominoCollection* TC, TetrominoData* TD, unsigned x, unsigned y) {
        TD->color = randColor();
        TD->rotation = randRotation();

        // Initialize position to center of screen
        TD->x = x;
        TD->y = y;

        TD->shape = &TC->tetrominos[rand() % TC->count]; // Chosing 1 of random tetrimino from the collection
}

static void InitializeBlocks(SandBlock* block, int x, int y) {
        block->x = x;
        block->y = y;
        block->velY = 0;
        block->color = randColor();
}

bool game_init(GameContext* GC) {
        srand(time(NULL)); // Seeding the random with current time
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
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        // Game Context Initialization
        GC->renderer = renderer;
        GC->window = window;
        GC->running = true;
        GC->redraw = true;
        GC->last_time = SDL_GetTicks();
        GC->delta_time = 0.0f;

        // Game Data Initialization
        GameData* GD = &GC->gameData;
        GD->gameOver = false;
        GD->level = 0;
        GD->score = 0;

        // Initialize Collection (1 time function so inline)
        InitializeTetriminoCollection(&GD->tetrominoCollection);

        // Initialize Current and Next Tetrimono
        unsigned x = 100, y = 100; // TODO: fix this to be in accordance to where to spawn
        InitializeTetriminoData(&GD->tetrominoCollection, &GD->currentTetromino, x, y);
        InitializeTetriminoData(&GD->tetrominoCollection, &GD->nextTetromino, x, y);

        // Setting up virtual resolution
        SDL_RenderSetLogicalSize(GC->renderer, VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
        SDL_RenderSetIntegerScale(GC->renderer, SDL_TRUE);
        return true;
}

void game_handle_events(GameContext* GC) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
                switch (event.type) {
                        case SDL_QUIT: {
                                GC->running = false;
                                break;
                        }

                        case SDL_KEYDOWN: {
                                switch (event.key.keysym.sym) {
                                        case SDLK_ESCAPE: {
                                                GC->running = false;
                                                break;
                                        }
                                }
                                break;
                        }
                }
        }
}

SandBlock SimulationBlock = {
        .x = (GAME_POS_X + GAME_WIDTH - PARTICLE_COUNT_IN_BLOCK_COLUMN) / 2.0f,
        .y = GAME_POS_Y + GAME_PADDING,
        .color = COLOR_RED,
        .velY = GRAVITY
};

void game_update(GameContext* GC) {
        SimulationBlock.velY += GRAVITY * GC->delta_time;
        SimulationBlock.y += SimulationBlock.velY * GC->delta_time;
        // printf("dt=%f velY=%f y=%f\n", GC->delta_time, SimulationBlock.velY, SimulationBlock.y);

        float floorY = VIRTUAL_HEIGHT - GAME_PADDING;
        if (SimulationBlock.y >= floorY - PARTICLE_COUNT_IN_BLOCK_ROW) {
                SimulationBlock.y = floorY - PARTICLE_COUNT_IN_BLOCK_ROW;

                if (fabsf(SimulationBlock.velY) < GRAVITY) {
                        SimulationBlock.velY = 0.0f;
                } else {
                        SimulationBlock.velY *= -0.5f;
                }
        }
}

static void renderSandBlock(SDL_Renderer* renderer, SandBlock* SB) {
        // TODO: set value of color in accordance to sandblock's particle color
        SDL_Rect SB_Rect = {
                .x = (int) SB->x,
                .y = (int) SB->y,
                .w = PARTICLE_COUNT_IN_BLOCK_COLUMN,
                .h = PARTICLE_COUNT_IN_BLOCK_ROW
        };
        SDL_RenderFillRect(renderer, &SB_Rect);
}

void game_render(GameContext* GC) {
        if (GC->redraw) {
                // Clear to BLACK
                SDL_SetRenderDrawColor(GC->renderer, 0, 0, 0, 255);
                SDL_RenderClear(GC->renderer);

                if (DEBUG) {
                        SDL_SetRenderDrawColor(GC->renderer, 255, 0, 0, 255);
                        SDL_Rect r = { 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT };
                        SDL_RenderDrawRect(GC->renderer, &r);

                        r.w = (r.w / 3) * 2;
                        SDL_RenderDrawRect(GC->renderer, &r);

                        r.w = GAME_WIDTH;
                        r.h = GAME_HEIGHT;
                        r.x = GAME_POS_X;
                        r.y = GAME_POS_Y;
                        SDL_RenderDrawRect(GC->renderer, &r);

                        renderSandBlock(GC->renderer, &SimulationBlock);
                }

                // Display modified renderer
                SDL_RenderPresent(GC->renderer);
                GC->redraw = false;
        }
}

void game_cleanup(GameContext* GC) {
        CleanUpTetriminoCollection(&GC->gameData.tetrominoCollection);

        SDL_DestroyRenderer(GC->renderer);
        SDL_DestroyWindow(GC->window);
        SDL_Quit();
}

static inline void InitializeTetriminoCollection(TetrominoCollection* TC) {
        TC->capacity = 4; // 4 Tetriminos: | Shaped, Z Shaped, Square Shaped, L Shape
        TC->tetrominos = malloc(sizeof(struct Tetromino) * TC->capacity);
        TC->count = 0;
        TC->tetrominos[TC->count++] = (struct Tetromino) {
                .name = "Line Tetrimino", // Display Name!
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
        // TODO: Other Blocks
}

static inline void CleanUpTetriminoCollection(TetrominoCollection* TC) {
        if (TC->tetrominos != NULL) {
                free(TC->tetrominos);
        }
}