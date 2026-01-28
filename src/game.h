#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <stdbool.h>
#include "config.h"

typedef enum {
        COLOR_RED,
        COLOR_GREEN,
        COLOR_BLUE,
        COLOR_COUNT
} ColorCode;

struct Tetromino {
        // This is constant structure for defining the shapes of tetromino: L Shape, Square Shape, Line, Z Shape...
        // A tetromino is a geometric shape composed of four connected squares
        // 4 different rotation options
        unsigned short shape[4][4][4];
        char name[32]; // Optional?
};
typedef struct {
        struct Tetromino* tetrominos;
        size_t capacity;
        size_t count;
} TetrominoCollection;

typedef struct {
        ColorCode color;
        // SandParticle particles[PARTICLE_COUNT_IN_BLOCK_ROW][PARTICLE_COUNT_IN_BLOCK_COLUMN];
        float x, y; // top-left position of the block
        float velY; // Velocity which determines how particle behaves!
} SandBlock;

typedef struct {
        const struct Tetromino *shape;
        uint8_t rotation; // 0–3
        unsigned x, y; // position of topleft block's topleft!
        ColorCode color;

        SandBlock sandBlock[4]; // 4 Blocks in a tetrimino
} TetrominoData;

typedef struct {
        // Data on all things needed for game to function
        unsigned score, level;
        uint8_t colorGrid[GAME_HEIGHT][GAME_WIDTH]; // Store color code only for all pixels on game screen (After blocks converted to sand)

        TetrominoCollection tetrominoCollection; // Total Tetromino type in game collection!

        TetrominoData currentTetromino;
        TetrominoData nextTetromino;

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

        // Gamedata: gameOver? score, level, sanddata, which tetromino next?, etc
        GameData gameData;

        // Redraw or not!
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
// After landing: move that to color Grid, initialize next tetromino...