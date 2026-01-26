#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include "config.h"
#include <stdbool.h>

typedef enum {
        COLOR_RED = 1,
        COLOR_GREEN,
        COLOR_BLUE
} ColorCode;

#define GRAVITY 10
typedef struct {
        // TODO maybe reimplement for collide logic?: Removed! unsigned x, y; Block contains its position, else GameData.colorGrid contains it once it is destroyed
        float velY, velX; // Rate of falling
        unsigned short colorCode; // Color of particle
} SandParticle;

struct Tetromino {
        // This is constant structure for defining the shapes of tetromino: L Shape, Square Shape, Line, Z Shape...
        // A tetromino is a geometric shape composed of four connected squares
        // 4 different rotation options
        unsigned short shape[4][4][4];
};
extern const struct Tetromino I_BLOCK; // Defined in game.c

#define PARTICLE_COUNT_IN_BLOCK_COLUMN 10
#define PARTICLE_COUNT_IN_BLOCK_ROW PARTICLE_COUNT_IN_BLOCK_COLUMN
typedef struct {
        SandParticle particles[PARTICLE_COUNT_IN_BLOCK_ROW][PARTICLE_COUNT_IN_BLOCK_COLUMN];
        unsigned x, y; // top-left position of the cell
} SandBlock;

typedef struct {
        const struct Tetromino *shape;
        uint8_t rotation; // 0–3
        unsigned x, y; // position in BLOCK units
        SandBlock blocks[4]; // 4 squares of particles
} ActiveTetromino;

#define GAME_HEIGHT (int) (0.9 * VIRTUAL_HEIGHT)
#define GAME_WIDTH (VIRTUAL_WIDTH / 3)
typedef struct {
        // Data on all things needed for game to function
        unsigned score, level;
        uint8_t colorGrid[GAME_HEIGHT][GAME_WIDTH]; // Store color code only for all pixels on game screen

        ActiveTetromino currentTetromino;
        ActiveTetromino nextTetromino;

        bool gameOver;
} GameData;

// Main game context
typedef struct {
        SDL_Window *window;
        SDL_Renderer *renderer;
        bool running;

        // Timing
        Uint32 last_time;
        float delta_time;

        // Game state
        GameData gameData;
        bool redraw;
} GameContext;

bool game_init(GameContext*);
void game_handle_events(GameContext*);
void game_update(GameContext*);
void game_render(GameContext*);
void game_cleanup(GameContext*);

#endif

// LOGIC
// While falling: active tetromino + particle Block
// After landing: move that to color Grid