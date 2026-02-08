#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdint.h>
#include "config.h"
#include "font.h"
#include "Audio.h"
#include "HighScore.h"

typedef enum {
        COLOR_RED = 0,
        COLOR_GREEN,
        COLOR_BLUE,
        COLOR_YELLOW,

        COLOR_COUNT,

        COLOR_DELETE_MARKED_SAND,

        COLOR_BORDER,
        COLOR_BACKGROUND, // Background color
        COLOR_SAND,

        COLOR_NONE, // Special Type!
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
        ColorCode color; // repeated in tetrominoData but who cares!
        // SandParticle particles[PARTICLE_COUNT_IN_BLOCK_ROW][PARTICLE_COUNT_IN_BLOCK_COLUMN];
        float x, y; // top-left position of the block
        float velY; // Velocity which determines how particle behaves!
} SandBlock;

typedef struct {
        const struct Tetromino *shape;
        uint8_t rotation; // 0–3
        float x, y; // position of topleft block's topleft!
        float velY;
        ColorCode color;

        // SandBlock sandBlock[4]; // 4 Blocks in a tetrimino
} TetrominoData;

typedef struct {
        // Data on all things needed for game to function
        unsigned score;

        int colorGrid[GAME_HEIGHT][GAME_WIDTH]; // Store color code only for all pixels on game screen (After blocks converted to sand)

        TetrominoCollection tetrominoCollection; // Total Tetromino type in game collection!

        TetrominoData currentTetromino;
        TetrominoData ghostTetromino;
        TetrominoData nextTetromino;

        bool gameOver;
        bool sandRemoveTrigger;
} GameData;

// Main game context
typedef struct {
        SDL_Window *window;
        SDL_Renderer *renderer;

        SDL_Texture* texture;
        SDL_PixelFormat *pixelFormat;

        bool running;

        int HIGH_SCORES[HIGH_SCORE_COUNT];

        // Timing
        Uint32 last_time;
        float delta_time;

        const Uint8* keys;

        // Gamedata: gameOver? score, level, sanddata, which tetromino next?, etc
        GameData gameData;
        AudioData audioData;
        FontData fontData;
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