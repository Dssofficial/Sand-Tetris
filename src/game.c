#include "game.h"
#include "config.h"
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_stdinc.h>
#include <stdint.h>
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

static void destroyCurrentTetromino(GameData* GD);

// Return Color for colorcode
static SDL_Color enumToColor(ColorCode CC);

// One time Function
static inline void InitializeTetriminoCollection(TetrominoCollection* TC);
static inline void CleanUpTetriminoCollection(TetrominoCollection* TC);

// This function called to create new tetrimino
// For currentTetrimino once in init
// For every nextTetrimino determination
static void InitializeTetriminoData(TetrominoCollection* TC, TetrominoData* TD, float x, float y) {
        TD->shape = &TC->tetrominos[rand() % TC->count]; // Chosing 1 of random tetrimino from the collection

        TD->color = randColor();
        TD->rotation = randRotation();
        TD->velY = 0;


        TD->x = x;
        TD->y = y;
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
        GC->last_time = SDL_GetTicks();
        GC->delta_time = 0.0f;
        GC->keys = SDL_GetKeyboardState(NULL);

        // Game Data Initialization
        GameData* GD = &GC->gameData;
        GD->gameOver = false;
        GD->level = 0;
        GD->score = 0;

        for (int i = 0; i < GAME_HEIGHT; i++) {
                for (int j = 0; j < GAME_WIDTH; j++) {
                        GD->colorGrid[i][j] = COLOR_NONE;
                }
        }

        // Initialize Collection (1 time function so inline)
        InitializeTetriminoCollection(&GD->tetrominoCollection);

        // Initialize Current and Next Tetrimono
        InitializeTetriminoData(
                &GD->tetrominoCollection,
                &GD->currentTetromino,
                GAME_POS_X + (GAME_WIDTH - 4.0f * PARTICLE_COUNT_IN_BLOCK_COLUMN) / 2.0f,
                GD->currentTetromino.y = - 2.0f * PARTICLE_COUNT_IN_BLOCK_ROW
        );
        InitializeTetriminoData(
                &GD->tetrominoCollection,
                &GD->nextTetromino,
                GAME_POS_X + GAME_WIDTH + GAME_PADDING + (1 / 3.0f * VIRTUAL_WIDTH - 4.0f * PARTICLE_COUNT_IN_BLOCK_COLUMN) / 2.0f,
                GAME_POS_Y + (GAME_HEIGHT - PARTICLE_COUNT_IN_BLOCK_ROW * 4.0f) / 2.0f
        );

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
                                                if (DEBUG) {
                                                        GC->running = false;
                                                }
                                                break;
                                        }

                                        case SDLK_UP: {
                                                GC->gameData.currentTetromino.rotation = (GC->gameData.currentTetromino.rotation + 1) % 4;
                                                break;
                                        }

                                        case SDLK_DOWN: {
                                                // TODO: Add velocity concept to this too
                                                GC->gameData.currentTetromino.y += GC->delta_time * 200;
                                                break;
                                        }

                                        case SDLK_d: {
                                                destroyCurrentTetromino(&GC->gameData);
                                                break;
                                        }
                                }
                                break;
                        }
                }
        }

        // TODO: Clamp position of tetrimino between game borders
        // Move Current Tetrimino
        if (GC->keys[SDL_SCANCODE_LEFT]) {
                GC->gameData.currentTetromino.x -= TETRIMINO_MOVE_SPEED * GC->delta_time;
        }
        if (GC->keys[SDL_SCANCODE_RIGHT]) {
                GC->gameData.currentTetromino.x += TETRIMINO_MOVE_SPEED * GC->delta_time;
        }
}

// static void update_sand_particle_falling(int (*colorGrid)[GAME_WIDTH]) {
//         // Bottom-up update
//         for (int y = GAME_HEIGHT - 1; y > 0; y--) {
//                 for (int x = 0; x < GAME_WIDTH; x++) {
//                         if (colorGrid[y][x] != COLOR_NONE) continue;

//                         // Check above
//                         if (colorGrid[y - 1][x] != COLOR_NONE) {
//                                 colorGrid[y][x] = colorGrid[y - 1][x];
//                                 colorGrid[y - 1][x] = COLOR_NONE;
//                                 continue;
//                         }

//                 }
//         }
// }
static void update_sand_particle_falling(int (*colorGrid)[GAME_WIDTH]) {
        // Process from bottom to top (second-to-bottom row up to top)
        for (int y = GAME_HEIGHT - 2; y >= 0; y--) {
                // Process each column
                for (int x = 0; x < GAME_WIDTH; x++) {
                        // Skip empty cells
                        if (colorGrid[y][x] == COLOR_NONE) {
                                continue;
                        }

                        // Check if cell below is empty
                        if (colorGrid[y + 1][x] == COLOR_NONE) {
                                // Move straight down
                                colorGrid[y + 1][x] = colorGrid[y][x];
                                colorGrid[y][x] = COLOR_NONE;
                                continue;
                        }

                        // Randomize which diagonal to try first
                        int try_left_first = rand() % 2;

                        if (try_left_first) {
                                // Try left diagonal first
                                if (x > 0 && colorGrid[y + 1][x - 1] == COLOR_NONE) {
                                        colorGrid[y + 1][x - 1] = colorGrid[y][x];
                                        colorGrid[y][x] = COLOR_NONE;
                                        continue;
                                }
                                // Then try right diagonal
                                if (x < GAME_WIDTH - 1 && colorGrid[y + 1][x + 1] == COLOR_NONE) {
                                        colorGrid[y + 1][x + 1] = colorGrid[y][x];
                                        colorGrid[y][x] = COLOR_NONE;
                                        continue;
                                }
                        } else {
                                // Try right diagonal first
                                if (x < GAME_WIDTH - 1 && colorGrid[y + 1][x + 1] == COLOR_NONE) {
                                        colorGrid[y + 1][x + 1] = colorGrid[y][x];
                                        colorGrid[y][x] = COLOR_NONE;
                                        continue;
                                }
                                // Then try left diagonal
                                if (x > 0 && colorGrid[y + 1][x - 1] == COLOR_NONE) {
                                        colorGrid[y + 1][x - 1] = colorGrid[y][x];
                                        colorGrid[y][x] = COLOR_NONE;
                                        continue;
                                }
                        }
                }
        }
}

void game_update(GameContext* GC) {
        GameData* GD = &GC->gameData;
        // TODO: Step 0: Maximum clearance algorithm, delete sand, ... score, level, ...
        update_sand_particle_falling(GD->colorGrid);

        // Steps
        // 1. Check if current tetromino is colliding
        //      1.a if it is, convert that to sand and add to colorGrid, swap currentTetromino to nextTetromino and spawn newTetromino for nextTetromino
        //              Note: make sure to set the x, y to different for new tetromino
        //      1.b if it's not, Update current tetromino's location

        // 1. b
        GD->currentTetromino.velY += GRAVITY * GC->delta_time;
        GD->currentTetromino.y += GD->currentTetromino.velY * GC->delta_time;
}

static void renderSandBlock(SDL_Renderer* renderer, SandBlock* SB) {
        SDL_SetRenderDrawColor(renderer, unpack_color(enumToColor(SB->color)));

        SDL_Rect SB_Rect = {
                .x = (int) SB->x,
                .y = (int) SB->y,
                .w = PARTICLE_COUNT_IN_BLOCK_COLUMN,
                .h = PARTICLE_COUNT_IN_BLOCK_ROW
        };
        SDL_RenderFillRect(renderer, &SB_Rect);
}

static void renderTetrimino(SDL_Renderer* renderer, const TetrominoData* t) {
        SDL_SetRenderDrawColor(renderer, unpack_color(enumToColor(t->color)));

        SandBlock sb = { .color = t->color, .velY = 0 };

        const unsigned short (*shape)[4] = t->shape->shape[t->rotation]; // Credit: ChatGPT, didn't know how to make such pointer

        for (int row = 0; row < 4; row++) {
                for (int col = 0; col < 4; col++) {
                        if (shape[row][col] == 0) {
                                continue;
                        }

                        sb.x = t->x + col * PARTICLE_COUNT_IN_BLOCK_COLUMN;
                        sb.y = t->y + row * PARTICLE_COUNT_IN_BLOCK_ROW;

                        renderSandBlock(renderer, &sb);
                }
        }
}

static void gameRenderDebug(SDL_Renderer* renderer, TetrominoData* TD) {
        if (!DEBUG) return;
        SDL_SetRenderDrawColor(renderer, unpack_color(enumToColor(COLOR_RED)));
        SDL_Rect r = { 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT };
        SDL_RenderDrawRect(renderer, &r);

        r.w = (r.w / 3) * 2;
        SDL_RenderDrawRect(renderer, &r);

        r.x = GAME_POS_X - 1;
        r.y = GAME_POS_Y - 1;
        r.w = GAME_WIDTH + 2;
        r.h = GAME_HEIGHT + 2;
        SDL_RenderDrawRect(renderer, &r);

        // Current Tetromino border
        r.x = TD->x;
        r.y = TD->y;
        r.w = PARTICLE_COUNT_IN_BLOCK_COLUMN * 4;
        r.h = PARTICLE_COUNT_IN_BLOCK_ROW * 4;
        SDL_RenderDrawRect(renderer, &r);
}

static void renderAllParticles(SDL_Renderer* renderer, int colorGrid[GAME_HEIGHT][GAME_WIDTH]) {
        // TODO: Optimize this!
        // Causes FPS to drop from stable 60 to ~30-39
        for (int y = 0; y < GAME_HEIGHT; y++) {
                for (int x = 0; x < GAME_WIDTH; x++) {
                        SDL_SetRenderDrawColor(renderer, unpack_color(enumToColor(colorGrid[y][x])));
                        SDL_RenderDrawPoint(renderer, GAME_POS_X + x, GAME_POS_Y + y);
                }
        }
}

void game_render(GameContext* GC) {
        // Clear to BLACK
        SDL_SetRenderDrawColor(GC->renderer, 0, 0, 0, 255);
        SDL_RenderClear(GC->renderer);


        renderAllParticles(GC->renderer, GC->gameData.colorGrid);

        renderTetrimino(GC->renderer, &GC->gameData.currentTetromino);
        renderTetrimino(GC->renderer, &GC->gameData.nextTetromino); // Part of UI:

        gameRenderDebug(GC->renderer, &GC->gameData.currentTetromino);
        // Display modified renderer
        SDL_RenderPresent(GC->renderer);
}

void game_cleanup(GameContext* GC) {
        CleanUpTetriminoCollection(&GC->gameData.tetrominoCollection);

        SDL_DestroyRenderer(GC->renderer);
        SDL_DestroyWindow(GC->window);
        SDL_Quit();
}

static inline void InitializeTetriminoCollection(TetrominoCollection* TC) {
        TC->capacity = 5; // 4 Tetriminos: | Shaped, Z Shaped, Square Shaped, L Shape
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

        TC->tetrominos[TC->count++] = (struct Tetromino) {
                .name = "Square Tetrimino", // Display Name!
                .shape = {
                        { // Rotation 1: rotation left of rotation 4
                                {0, 0, 0, 0},
                                {0, 1, 1, 0},
                                {0, 1, 1, 0},
                                {0, 0, 0, 0},
                        },
                        { // Rotation 2: rotation left of rotation 1
                                {0, 0, 0, 0},
                                {0, 1, 1, 0},
                                {0, 1, 1, 0},
                                {0, 0, 0, 0},
                        },
                        { // Rotation 3: rotation left of rotation 2
                                {0, 0, 0, 0},
                                {0, 1, 1, 0},
                                {0, 1, 1, 1},
                                {0, 0, 0, 0},
                        },
                        { // Rotation 4: rotation left of rotation 3
                                {0, 0, 0, 0},
                                {0, 1, 1, 0},
                                {0, 1, 1, 0},
                                {0, 0, 0, 0},
                        }
                }
        };

        TC->tetrominos[TC->count++] = (struct Tetromino) {
                .name = "Skew Tetrimino", // Display Name!
                .shape = {
                        { // Rotation 1: rotation left of rotation 4
                                {0, 0, 0, 0},
                                {0, 0, 1, 1},
                                {0, 1, 1, 0},
                                {0, 0, 0, 0},
                        },
                        { // Rotation 2: rotation left of rotation 1
                                {0, 1, 0, 0},
                                {0, 1, 1, 0},
                                {0, 0, 1, 0},
                                {0, 0, 0, 0},
                        },
                        { // Rotation 3: rotation left of rotation 2
                                {0, 0, 0, 0},
                                {0, 1, 1, 0},
                                {1, 1, 0, 0},
                                {0, 0, 0, 0},
                        },
                        { // Rotation 4: rotation left of rotation 3
                                {0, 0, 0, 0},
                                {0, 1, 0, 0},
                                {0, 1, 1, 0},
                                {0, 0, 1, 0},
                        }
                }
        };

        TC->tetrominos[TC->count++] = (struct Tetromino) {
                .name = "L Tetrimino", // Display Name!
                .shape = {
                        { // Rotation 1: rotation left of rotation 4
                                {0, 0, 0, 0},
                                {0, 1, 0, 0},
                                {0, 1, 0, 0},
                                {0, 1, 1, 0},
                        },
                        { // Rotation 2: rotation left of rotation 1
                                {0, 0, 0, 0},
                                {0, 0, 0, 1},
                                {0, 1, 1, 1},
                                {0, 0, 0, 0},
                        },
                        { // Rotation 3: rotation left of rotation 2
                                {0, 1, 1, 0},
                                {0, 0, 1, 0},
                                {0, 0, 1, 0},
                                {0, 0, 0, 0},
                        },
                        { // Rotation 4: rotation left of rotation 3
                                {0, 0, 0, 0},
                                {1, 1, 1, 0},
                                {1, 0, 0, 0},
                                {0, 0, 0, 0},
                        }
                }
        };


        TC->tetrominos[TC->count++] = (struct Tetromino) {
                .name = "T Tetrimino", // Display Name!
                .shape = {
                        { // Rotation 1: rotation left of rotation 4
                                {0, 0, 0, 0},
                                {0, 1, 1, 1},
                                {0, 0, 1, 0},
                                {0, 0, 1, 0},
                        },
                        { // Rotation 2: rotation left of rotation 1
                                {0, 1, 0, 0},
                                {0, 1, 1, 1},
                                {0, 1, 0, 0},
                                {0, 0, 0, 0},
                        },
                        { // Rotation 3: rotation left of rotation 2
                                {0, 1, 0, 0},
                                {0, 1, 0, 0},
                                {1, 1, 1, 0},
                                {0, 0, 0, 0},
                        },
                        { // Rotation 4: rotation left of rotation 3
                                {0, 0, 0, 0},
                                {0, 0, 1, 0},
                                {1, 1, 1, 0},
                                {0, 0, 1, 0},
                        }
                }
        };
}

static inline void CleanUpTetriminoCollection(TetrominoCollection* TC) {
        if (TC->tetrominos != NULL) {
                free(TC->tetrominos);
        }
}

static SDL_Color enumToColor(ColorCode CC){
        SDL_Color color = {0};

        switch (CC) {
                case COLOR_RED: {
                        color = (SDL_Color) {
                                .r = 255,
                                .g = 0,
                                .b = 0,
                                .a = 255
                        };
                        break;
                }


                case COLOR_GREEN: {
                        color = (SDL_Color) {
                                .r = 0,
                                .g = 255,
                                .b = 0,
                                .a = 255
                        };
                        break;
                }

                case COLOR_BLUE: {
                        color = (SDL_Color) {
                                .r = 0,
                                .g = 0,
                                .b = 255,
                                .a = 255
                        };
                        break;
                }

                case COLOR_NONE: {
                        color = (SDL_Color) {
                                .r = 0,
                                .g = 0,
                                .b = 0,
                                .a = 0
                        };
                };

                default: {
                        color = (SDL_Color) {
                                .r = 0,
                                .g = 0,
                                .b = 0,
                                .a = 255
                        };
                };
        }

        return color;
}

static void destroyCurrentTetromino(GameData* GD) {
        TetrominoData *TD = &GD->currentTetromino;
        const unsigned short (*shape)[4] = TD->shape->shape[TD->rotation];

        ColorCode color = TD->color;
        int start_x = (int)TD->x - GAME_POS_X;
        int start_y = (int)TD->y - GAME_POS_Y;

        // Loop through 4x4 grid of current tetromino
        for (int row = 0; row < 4; row++) {
                for (int col = 0; col < 4; col++) {
                        if (shape[row][col] == 0) continue;

                        // Calculate base position for this block within the tetromino
                        int block_base_x = start_x + col * PARTICLE_COUNT_IN_BLOCK_COLUMN;
                        int block_base_y = start_y + row * PARTICLE_COUNT_IN_BLOCK_ROW;

                        for (int y_offset = 0; y_offset < PARTICLE_COUNT_IN_BLOCK_ROW; y_offset++) {
                                int grid_y = block_base_y + y_offset;

                                // Check if within vertical bounds
                                if (grid_y < 0 || grid_y >= GAME_HEIGHT) continue; // temporary

                                for (int x_offset = 0; x_offset < PARTICLE_COUNT_IN_BLOCK_COLUMN; x_offset++) {
                                        int grid_x = block_base_x + x_offset;

                                        // Check if within horizontal bounds
                                        if (grid_x < 0 || grid_x >= GAME_WIDTH) continue; // temporary

                                        // Place the color in the grid
                                        GD->colorGrid[grid_y][grid_x] = color;
                                }
                        }
                }
        }

        // Swap current and next tetromino
        GD->currentTetromino = GD->nextTetromino;

        // Reset position and velocity for the new current tetromino
        GD->currentTetromino.x = GAME_POS_X + (GAME_WIDTH - 4.0f * PARTICLE_COUNT_IN_BLOCK_COLUMN) / 2.0f;
        GD->currentTetromino.y = - 2.0f * PARTICLE_COUNT_IN_BLOCK_ROW;
        GD->currentTetromino.velY = 0;

        // Initialize new next tetromino
        InitializeTetriminoData(
                &GD->tetrominoCollection,
                &GD->nextTetromino,
                GAME_POS_X + GAME_WIDTH + GAME_PADDING + (1/3.0f * VIRTUAL_WIDTH - 4.0f * PARTICLE_COUNT_IN_BLOCK_COLUMN) / 2.0f,
                GAME_POS_Y + (GAME_HEIGHT - PARTICLE_COUNT_IN_BLOCK_ROW * 4.0f) / 2.0f
        );
}