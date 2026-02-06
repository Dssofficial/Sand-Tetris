#include "game.h"
#include "config.h"
#include "font.h"
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_ttf.h>
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
#include <math.h>
#include <stdlib.h>

#define unpack_color(color) (color.r), (color.g), (color.b), (color.a)
#define randColor() (rand() % COLOR_COUNT)
#define randRotation() (rand() % 4) // 4 rotations total so MAGIC NUMBER

// One time Function
static inline void InitializeTetriminoCollection(TetrominoCollection* TC);
static inline void CleanUpTetriminoCollection(TetrominoCollection* TC);

// Other functions
static void destroyCurrentTetromino(GameData* GD);
static SDL_Color enumToColor(ColorCode CC);
static SDL_Rect TetrominoBounds(TetrominoData* TD);

static void renderTetrimino(SDL_Renderer* renderer, const TetrominoData* t);

static void gameRenderDebug(SDL_Renderer* renderer, TetrominoData* TD) {
        if (!DEBUG) return;
        SDL_SetRenderDrawColor(renderer, unpack_color(enumToColor(COLOR_RED)));

        SDL_Rect r = {
                .x = TD->x,
                .y = TD->y,
                .w = PARTICLE_COUNT_IN_BLOCK_COLUMN * 4,
                .h = PARTICLE_COUNT_IN_BLOCK_ROW * 4
        };
        SDL_RenderDrawRect(renderer, &r);

        // Current Tetromino border
        r = TetrominoBounds(TD);
        SDL_SetRenderDrawColor(renderer, unpack_color(enumToColor(COLOR_GREEN)));
        SDL_RenderDrawRect(renderer, &r);
}


// This function called to create new tetrimino
// For currentTetrimino once in init
// For every nextTetrimino determination
static void InitializeTetriminoData(TetrominoCollection* TC, TetrominoData* TD) {
        TD->shape = &TC->tetrominos[rand() % TC->count]; // Chosing 1 of random tetrimino from the collection

        TD->color = randColor();
        TD->rotation = randRotation();
        TD->velY = 0;

        TD->x = 0;
        TD->y = 0;
}

bool game_init(GameContext* GC) {
        srand(time(NULL)); // Seeding the random with current time
        if (SDL_Init(SDL_INIT_VIDEO) != 0) { // SDL_INIT_AUDIO, maybe even timer?
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

        SDL_Texture* texture = SDL_CreateTexture(
                renderer,
                SDL_PIXELFORMAT_RGBA8888,
                SDL_TEXTUREACCESS_STREAMING,
                GAME_WIDTH, GAME_HEIGHT
        );
        if (!texture) {
                fprintf(stderr, "Texture error: %s\n", SDL_GetError());
                SDL_DestroyRenderer(renderer);
                SDL_DestroyWindow(window);
                SDL_Quit();
                return false;
        }

        Uint32 fmt;
        SDL_QueryTexture(texture, &fmt, NULL, NULL, NULL);
        if (!fmt) {
                fprintf(stderr, "SDL_PixelFormat error: %s\n", SDL_GetError());
                SDL_DestroyRenderer(renderer);
                SDL_DestroyWindow(window);
                SDL_Quit();
                return false;
        }
        SDL_assert(fmt == SDL_PIXELFORMAT_RGBA8888); // Did a mistake once so just assurance assert for my hart

        if (TTF_Init() == -1) {
                fprintf(stderr, "TTF_Init error: %s\n", SDL_GetError());
                goto something;
        };
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

        FontData fontData;
        if (fontData_init(&fontData) == -1) {
                fprintf(stderr, "FontData Initialization Error!\n");

                something: {
                        SDL_DestroyTexture(texture);
                        SDL_DestroyRenderer(renderer);
                        SDL_DestroyWindow(window);
                        SDL_Quit();
                        return false;
                }
        }

        // Game Context Initialization
        GC->window = window;
        GC->renderer = renderer;
        GC->texture = texture;
        GC->fontData = fontData;
        GC->pixelFormat = SDL_AllocFormat(fmt);
        GC->running = true;
        GC->last_time = SDL_GetTicks();
        GC->delta_time = 0.0f;
        GC->keys = SDL_GetKeyboardState(NULL);

        // Game Data Initialization
        GameData* GD = &GC->gameData;
        GD->gameOver = false;
        GD->sandRemoveTrigger = false;
        GD->level = 0;
        GD->score = 0;
        GD->totalSandCleared = 0;

        for (int i = 0; i < GAME_HEIGHT; i++) {
                for (int j = 0; j < GAME_WIDTH; j++) {
                        GD->colorGrid[i][j] = COLOR_NONE; // Fun: rand() %2 ==0?randColor(): COLOR_NONE;
                }
        }

        // Initialize Collection (1 time function so inline)
        InitializeTetriminoCollection(&GD->tetrominoCollection);

        // Initialize Current and Next Tetrimono
        InitializeTetriminoData(&GD->tetrominoCollection, &GD->currentTetromino);
        SDL_Rect rect = TetrominoBounds(&GD->currentTetromino);
        GD->currentTetromino.x = GAME_POS_X + (GAME_WIDTH - rect.w) * 0.5f - rect.x;
        GD->currentTetromino.y = GAME_POS_Y - rect.h;

        InitializeTetriminoData(&GD->tetrominoCollection, &GD->nextTetromino);
        rect = TetrominoBounds(&GD->nextTetromino);
        GD->nextTetromino.x = INFO_PANEL_X + (INFO_PANEL_WIDTH - rect.w) * 0.5f - rect.x;
        GD->nextTetromino.y = INFO_PANEL_Y + (INFO_PANEL_HEIGHT) * 0.1f;

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
                                                if (GC->gameData.gameOver) {
                                                        return;
                                                }
                                                GC->gameData.currentTetromino.rotation = (GC->gameData.currentTetromino.rotation + 1) % 4;
                                                break;
                                        }

                                        case SDLK_DOWN: {
                                                if (GC->gameData.gameOver) {
                                                        return;
                                                }

                                                if (GC->gameData.currentTetromino.rotation > 0) {
                                                        GC->gameData.currentTetromino.rotation = GC->gameData.currentTetromino.rotation - 1;
                                                } else {
                                                        GC->gameData.currentTetromino.rotation = 3;
                                                }
                                                break;
                                        }

                                        case SDLK_SPACE: {
                                                if (GC->gameData.gameOver) {
                                                        return;
                                                }
                                                if (GC->gameData.currentTetromino.y < 0) break;

                                                // Move to renderer:


                                                // TODO: Fix collision detection so that this works
                                                //      Alternative solution: find the pixel closest to it vertically down, and add y difference to it's y
                                                // GC->gameData.currentTetromino.velY += 100;
                                                break;
                                        }

                                }
                                break;
                        }
                }
        }

        if (GC->gameData.gameOver) {
                // TODO: event handle: "space/enter" for restart game?
                return;
        }

        // Move Current Tetrimino
        // TODO: smoother control
        TetrominoData* TD = &GC->gameData.currentTetromino;
        if (GC->keys[SDL_SCANCODE_LEFT]) {
                TD->x -= TETRIMINO_MOVE_SPEED * GC->delta_time;
        }
        if (GC->keys[SDL_SCANCODE_RIGHT]) {
                TD->x += TETRIMINO_MOVE_SPEED * GC->delta_time;
        }

        // Clamp position: to inbetween walls
        int minCol = 4;
        int maxCol = -1;
        const unsigned short (*shape)[4] = TD->shape->shape[TD->rotation];
        for (int row = 0; row < 4; row++) {
                for (int col = 0; col < 4; col++) {
                        if (shape[row][col]) {
                                if (col < minCol) minCol = col;
                                if (col > maxCol) maxCol = col;
                        }
                }
        }

        int minX = GAME_POS_X + -minCol * PARTICLE_COUNT_IN_BLOCK_COLUMN;
        int maxX = GAME_POS_X + GAME_WIDTH - (maxCol + 1) * PARTICLE_COUNT_IN_BLOCK_COLUMN;
        TD->x = SDL_clamp(TD->x, minX, maxX);
}

// Also Updates score
static void removeParticlesGracefully(GameContext* GC) {
        int (*colorGrid)[GAME_WIDTH] = GC->gameData.colorGrid;
        static float timer = 0.0f;
        timer += GC->delta_time;
        unsigned score = 0;

        if (timer > fmax(0.25f, (TIME_FOR_SAND_DELETION / (GC->gameData.level / 10.0f + 1)))) {
                for (int y = 0; y < GAME_HEIGHT; y++) {
                        for (int x = 0; x < GAME_WIDTH; x++) {
                                if (colorGrid[y][x] == COLOR_DELETE_MARKED_SAND) {
                                        colorGrid[y][x] = COLOR_NONE;
                                        score++;
                                }
                        }
                }

                unsigned finalScore = (unsigned)(score * log((float) score + 1));
                GC->gameData.score += finalScore;
                GC->gameData.totalSandCleared += score;

                if (GC->gameData.level < 5) {
                        GC->gameData.level = GC->gameData.totalSandCleared / 500;
                } else {
                        GC->gameData.level = 5 + (unsigned)(logf((float)(GC->gameData.totalSandCleared - 2500) / 500.0f + 1.0f) / logf(1.6f));
                }

                // Additinal Reward for scoring: half the current falling tetrimino falling
                GC->gameData.currentTetromino.velY = fmaxf(GC->gameData.currentTetromino.velY * 0.5f, GRAVITY);

                GC->gameData.sandRemoveTrigger = false;
                timer = 0.0f;
        }
}

static bool update_sand_particle_falling(int (*colorGrid)[GAME_WIDTH], float deltaTime, int level) {
        static float sandAccumulator = 0.0f;
        sandAccumulator += deltaTime;

        bool returnValue = false; // whether sands that need to be removed is in the colorGrid

        while (sandAccumulator >= SAND_STEP_TIME) { // Move the level, faster sand falls cause for fun!
                sandAccumulator -= fmax(SAND_STEP_TIME * 1 / 2.5f, (SAND_STEP_TIME / (level / 10.0f + 1)));

                // Process from bottom to top (second-to-bottom row up to top)
                for (int y = GAME_HEIGHT - 2; y >= 0; y--) {
                        // Process each column
                        for (int x = 0; x < GAME_WIDTH; x++) {
                                // Skip empty cells
                                if (colorGrid[y][x] == COLOR_NONE) {
                                        continue;
                                } else if (colorGrid[y][x] == COLOR_DELETE_MARKED_SAND) {
                                        returnValue = true;
                                        continue;
                                }

                                // Check if cell below is empty
                                if (colorGrid[y + 1][x] == COLOR_NONE) {
                                        // Move straight down
                                        colorGrid[y + 1][x] = colorGrid[y][x];
                                        colorGrid[y][x] = COLOR_NONE;
                                        continue;
                                }

                                int try_left_first = rand() % 2;
                                if (try_left_first) {
                                        if (x > 0 && colorGrid[y + 1][x - 1] == COLOR_NONE) {
                                                colorGrid[y + 1][x - 1] = colorGrid[y][x];
                                                colorGrid[y][x] = COLOR_NONE;
                                                continue;
                                        }
                                        if (x < GAME_WIDTH - 1 && colorGrid[y + 1][x + 1] == COLOR_NONE) {
                                                colorGrid[y + 1][x + 1] = colorGrid[y][x];
                                                colorGrid[y][x] = COLOR_NONE;
                                                continue;
                                        }
                                } else {
                                        if (x < GAME_WIDTH - 1 && colorGrid[y + 1][x + 1] == COLOR_NONE) {
                                                colorGrid[y + 1][x + 1] = colorGrid[y][x];
                                                colorGrid[y][x] = COLOR_NONE;
                                                continue;
                                        }
                                        if (x > 0 && colorGrid[y + 1][x - 1] == COLOR_NONE) {
                                                colorGrid[y + 1][x - 1] = colorGrid[y][x];
                                                colorGrid[y][x] = COLOR_NONE;
                                                continue;
                                        }
                                }
                        }
                }
        }
        return returnValue;
}

static bool checkIfTetriminoCollidingWithFloor(TetrominoData* TD) {
        // Check if tetromino collides with floor, snap to floor it true
        int maxRow = -1;

        const unsigned short (*shape)[4] = TD->shape->shape[TD->rotation];
        for (int row = 0; row < 4; row++) {
                for (int col = 0; col < 4; col++) {
                        if (shape[row][col]) {
                                if (row > maxRow) maxRow = row;
                        }
                }
        }

        int maxY = GAME_POS_Y + GAME_HEIGHT;
        int pieceBottom = TD->y + (maxRow + 1) * PARTICLE_COUNT_IN_BLOCK_ROW;
        if (pieceBottom > maxY) {
                TD->y -= (pieceBottom - maxY);
                return true;
        }
        return false;
}

static bool checkIfTetriminoCollidesWithOtherParticles(GameData* GD) {
        // TODO: Fix it
        TetrominoData* TD = &GD->currentTetromino;
        const unsigned short (*shape)[4] = TD->shape->shape[TD->rotation];

        for (int row = 0; row < 4; row++) {
                for (int col = 0; col < 4; col++) {
                        if (!shape[row][col] || (row < 3 && shape[row + 1][col])) {
                                continue;
                        }

                        int blockBaseX = TD->x + col * PARTICLE_COUNT_IN_BLOCK_COLUMN;
                        int blockBaseY = TD->y + row * PARTICLE_COUNT_IN_BLOCK_ROW;

                        for (int yOff = 0; yOff < PARTICLE_COUNT_IN_BLOCK_ROW; yOff++) {
                                int gridY = blockBaseY + yOff;

                                if (gridY < 0 || gridY >= GAME_HEIGHT) {
                                        continue;
                                }

                                for (int xOff = 0; xOff < PARTICLE_COUNT_IN_BLOCK_COLUMN; xOff++) {
                                        int gridX = blockBaseX + xOff;

                                        if (gridX < 0 || gridX >= GAME_WIDTH) {
                                                continue;
                                        }

                                        if (GD->colorGrid[gridY][gridX] != COLOR_NONE) {
                                                return true;
                                        }
                                }
                        }
                }
        }

        return false;
}

static bool floodFillDetectDiagonal(int grid[GAME_HEIGHT][GAME_WIDTH], bool visited[GAME_HEIGHT][GAME_WIDTH], int x, int y, ColorCode color) {
        // Recurive function that somehow works! (TODO: might have some edge cases, check and fix that)
        if (x < 0 || x >= GAME_WIDTH || y < 0 || y >= GAME_HEIGHT) {
                return false;
        }

        if (visited[y][x] || (grid[y][x] != color)) {
                return false;
        }

        visited[y][x] = true;
        bool reachesRight = (x == GAME_WIDTH - 1); // check it any of the particles of same color connected have reased the end

        // 8 directions
        for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dy == 0) {
                                continue;
                        }

                        reachesRight =  reachesRight | floodFillDetectDiagonal(grid, visited, x + dx, y + dy, color);
                }
        }

        return reachesRight;
}

static void sandClearance(GameData* GD) {
        int (*grid)[GAME_WIDTH] = GD->colorGrid;

        for (ColorCode color = 0; color < COLOR_COUNT; color++) {
                for (int y = 0; y < GAME_HEIGHT; y++) {
                        if (grid[y][0] != color) {
                                continue;
                        }

                        bool visited[GAME_HEIGHT][GAME_WIDTH] = {0};

                        bool spansLeftToRight = floodFillDetectDiagonal(grid, visited, 0, y, color);

                        if (spansLeftToRight) {
                                for (int yy = 0; yy < GAME_HEIGHT; yy++) {
                                        for (int xx = 0; xx < GAME_WIDTH; xx++) {
                                                if (visited[yy][xx]) {
                                                        grid[yy][xx] = COLOR_DELETE_MARKED_SAND;
                                                }
                                        }
                                }
                        }
                }
        }
}

static void checkIfGameOver(GameData* GD) {
        // if sand reaches a hight more than container
        for (int x = 0; x < GAME_WIDTH; x++) {
                if (GD->colorGrid[1][x] != COLOR_NONE) {
                        GD->gameOver = true;
                        return;
                }
        }
};

void game_update(GameContext* GC) {
        GameData* GD = &GC->gameData;
        TetrominoData* TD = &GD->currentTetromino;

        checkIfGameOver(GD);
        if (GD->gameOver) {
                // TODO
                return;
        }

        if (update_sand_particle_falling(GD->colorGrid, GC->delta_time, GD->level)) {
                GD->sandRemoveTrigger = true;
        };

        if (GD->sandRemoveTrigger) {
                removeParticlesGracefully(GC);
        }

        // Steps
        // 1. Check if current tetromino is colliding
        //      1.a if it is, convert that to sand and add to colorGrid, swap currentTetromino to nextTetromino and spawn newTetromino for nextTetromino
        //              Note: make sure to set the x, y to different for new tetromino
        //      1.b if it's not, Update current tetromino's location
        if (checkIfTetriminoCollidingWithFloor(TD) || checkIfTetriminoCollidesWithOtherParticles(GD)) {
                // 1.a
                destroyCurrentTetromino(GD);
        } else {
                // 1. b
                TD->velY += fmin(GRAVITY * GC->delta_time * 5, GRAVITY * (GC->gameData.level / 10.0f + 1) * GC->delta_time);
                TD->y += TD->velY * GC->delta_time;
        }

        // Maximum clearance algorithm, delete sand, ... score, level, ...
        // 2. Maximum clearance
        //      2.a Detect
        //      2.b Convert all to color_none gracefully i.e go from COLOR_DELETE_MARKED_SAND to COLOR_NONE
        sandClearance(GD);
}

static void renderSandBlock(SDL_Renderer* renderer, SandBlock* SB) {
        // Border!
        SDL_Rect SB_Rect = {
                .x = fmax((int) SB->x, GAME_POS_X),
                .y = fmax((int) SB->y, GAME_POS_Y),
                .w = PARTICLE_COUNT_IN_BLOCK_COLUMN,
                .h = PARTICLE_COUNT_IN_BLOCK_ROW
        };
        SDL_SetRenderDrawColor(renderer, unpack_color(enumToColor(COLOR_DELETE_MARKED_SAND)));
        SDL_RenderDrawRect(renderer, &SB_Rect);

        // Sand
        SB_Rect.x += 1;
        SB_Rect.y += 1;
        SB_Rect.w -= 2;
        SB_Rect.h -= 2;
        SDL_SetRenderDrawColor(renderer, unpack_color(enumToColor(SB->color)));
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

static SDL_Rect TetrominoBounds(TetrominoData* TD) {
        // Boundary
        int minCol = 4;
        int maxCol = -1;
        int minRow = 4;
        int maxRow = -1;

        const unsigned short (*shape)[4] = TD->shape->shape[TD->rotation];
        for (int row = 0; row < 4; row++) {
                for (int col = 0; col < 4; col++) {
                        if (shape[row][col]) {
                                if (col < minCol) minCol = col;
                                if (col > maxCol) maxCol = col;
                                if (row < minRow) minRow = row;
                                if (row > maxRow) maxRow = row;
                        }
                }
        }

        int pieceLeft = TD->x + minCol * PARTICLE_COUNT_IN_BLOCK_COLUMN;
        int pieceRight = TD->x + (maxCol + 1) * PARTICLE_COUNT_IN_BLOCK_COLUMN;

        int pieceTop = TD->y + minRow * PARTICLE_COUNT_IN_BLOCK_ROW;
        int pieceBottom = TD->y + (maxRow + 1) * PARTICLE_COUNT_IN_BLOCK_ROW;

        return (SDL_Rect) {
                .x = pieceLeft,
                .w = pieceRight - pieceLeft,
                .y = pieceTop,
                .h = pieceBottom - pieceTop,
        };
}

static void renderAllParticles(GameContext* GC) {
        void* pixels;
        int pitch;

        SDL_LockTexture(GC->texture, NULL, &pixels, &pitch);

        // pixels = raw RGBA buffer
        Uint32 *p = (Uint32 *)pixels;
        int pitch32 = pitch / sizeof(Uint32);
        Uint32 rgba;
        SDL_Color color;

        SDL_Color color_for_delete_marked_sand_defined = enumToColor(COLOR_DELETE_MARKED_SAND);

        int randValue = rand() % 2 == 0 ? 1: -1;
        SDL_Color color_for_delete_marked_sand = {
                .r = color_for_delete_marked_sand_defined.r + randValue * rand() % 50,
                .g = color_for_delete_marked_sand_defined.g + randValue * rand() % 50,
                .b = color_for_delete_marked_sand_defined.b + randValue * rand() % 50,
                .a = 255
        };

        for (int y = 0; y < GAME_HEIGHT; y++) {
                for (int x = 0; x < GAME_WIDTH; x++) {
                        if (GC->gameData.colorGrid[y][x] == COLOR_DELETE_MARKED_SAND) {
                                rgba = (color_for_delete_marked_sand.r << 24) | (color_for_delete_marked_sand.g << 16) | (color_for_delete_marked_sand.b << 8) | color.a;
                        } else {
                                if (GC->gameData.colorGrid[y][x] == COLOR_NONE) {
                                        color = enumToColor(COLOR_SAND);
                                } else {
                                        color = enumToColor(GC->gameData.colorGrid[y][x]);
                                }
                                rgba = (color.r << 24) | (color.g << 16) | (color.b << 8) | color.a;
                        }
                        p[y * pitch32 + x] = rgba;
                }
        }

        SDL_UnlockTexture(GC->texture);
        SDL_Rect dst = {
                GAME_POS_X,
                GAME_POS_Y,
                GAME_WIDTH,
                GAME_HEIGHT
        };

        SDL_RenderCopy(GC->renderer, GC->texture, NULL, &dst);
}

static void renderGameUI(SDL_Renderer* renderer, GameContext* GC) {
        SDL_SetRenderDrawColor(renderer, unpack_color(enumToColor(COLOR_BORDER)));

        SDL_Rect r = { 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT };
        SDL_RenderDrawRect(renderer, &r);

        r.w = (r.w / 3) * 2;
        SDL_RenderDrawRect(renderer, &r);

        r = (SDL_Rect) {
                .x = GAME_POS_X - 1,
                .y = GAME_POS_Y - 1,
                .w = GAME_WIDTH + 2,
                .h = GAME_HEIGHT + 2,
        };
        SDL_RenderDrawRect(renderer, &r);
        renderTetrimino(GC->renderer, &GC->gameData.nextTetromino);

        if (GC->gameData.gameOver) {
                // TODO
                // Display Score + Other infos
        }

        char str[256];
        SDL_Rect txtContainerRect;

        txtContainerRect = (SDL_Rect) {
                .x = INFO_PANEL_X + GAME_PADDING,
                .y = GC->gameData.nextTetromino.y + PARTICLE_COUNT_IN_BLOCK_ROW * 4,
                .w = INFO_PANEL_WIDTH - GAME_PADDING * 2,
                .h = 20 * SCALE_FACTOR,
        };
        snprintf(str, sizeof(str), "Next: %17s", GC->gameData.currentTetromino.shape->name);
        font_render_rect(&GC->fontData, GC->renderer, str, FONT_PATH, -1, TTF_STYLE_BOLD, enumToColor(COLOR_BORDER), txtContainerRect);
        SDL_RenderDrawRect(renderer, &txtContainerRect);

        // SCORE
        snprintf(str, sizeof(str), "Score: %15d", GC->gameData.score);
        txtContainerRect.y += txtContainerRect.h;
        font_render_rect(&GC->fontData, GC->renderer, str, FONT_PATH, -1, TTF_STYLE_NORMAL, enumToColor(COLOR_BORDER), txtContainerRect);
        // LEVEL
        txtContainerRect.y += txtContainerRect.h;
        snprintf(str, sizeof(str), "Level: %15d", GC->gameData.level);
        font_render_rect(&GC->fontData, GC->renderer, str, FONT_PATH, -1, TTF_STYLE_NORMAL, enumToColor(COLOR_BORDER), txtContainerRect);

        // HIGH SCORES
        // snprintf(str, sizeof(str), "High Scores: %d", GC->gameData.score);
        //
}

void game_render(GameContext* GC) {
        // Clear to BLACK
        SDL_SetRenderDrawColor(GC->renderer, 0, 0, 0, 255);
        SDL_RenderClear(GC->renderer);

        // Virtual ... Background:
        SDL_SetRenderDrawColor(GC->renderer, unpack_color(enumToColor(COLOR_BACKGROUND)));
        SDL_Rect r = { 0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT };
        SDL_RenderFillRect(GC->renderer, &r);

        // Game UI
        renderGameUI(GC->renderer, GC);

        // Game
        renderAllParticles(GC);
        renderTetrimino(GC->renderer, &GC->gameData.currentTetromino);

        // Debug:
        gameRenderDebug(GC->renderer, &GC->gameData.currentTetromino);

        // Display modified renderer
        SDL_RenderPresent(GC->renderer);
}

void game_cleanup(GameContext* GC) {
        CleanUpTetriminoCollection(&GC->gameData.tetrominoCollection);
        fontData_destroy(&GC->fontData);
        TTF_Quit();
        SDL_FreeFormat(GC->pixelFormat);
        SDL_DestroyTexture(GC->texture);
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
        {0, 1, 1, 0},
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
                                .r = 240,
                                .g = 0,
                                .b = 0,
                                .a = 255
                        };
                        break;
                }


                case COLOR_GREEN: {
                        color = (SDL_Color) {
                                .r = 0,
                                .g = 240,
                                .b = 0,
                                .a = 255
                        };
                        break;
                }

                case COLOR_BLUE: {
                        color = (SDL_Color) {
                                .r = 0,
                                .g = 0,
                                .b = 240,
                                .a = 255
                        };
                        break;
                }

                case COLOR_YELLOW: {
                        color = (SDL_Color) {
                                .r = 255,
                                .g = 255,
                                .b = 0,
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
                        break;
                };


                case COLOR_BORDER:
                case COLOR_DELETE_MARKED_SAND: {
                        color = (SDL_Color) {
                                .r = 217,
                                .g = 219,
                                .b = 206,
                                .a = 255
                        };
                        break;
                };

                case COLOR_BACKGROUND: {
                        color = (SDL_Color) {
                                .r = 15,
                                .g = 20,
                                .b = 25,
                                .a = 255
                        };
                        break;
                }

                case COLOR_SAND: {
                        color = (SDL_Color) {
                                .r = 76,
                                .g = 70,
                                .b = 50,
                                .a = 255
                        };
                        break;
                }

                default: {
                        color = (SDL_Color) { // Wall! i.e black
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
                                if (grid_y < 0 || grid_y >= GAME_HEIGHT) {
                                        continue;
                                }

                                for (int x_offset = 0; x_offset < PARTICLE_COUNT_IN_BLOCK_COLUMN; x_offset++) {
                                        int grid_x = block_base_x + x_offset;

                                        // Check if within horizontal bounds
                                        if (grid_x < 0 || grid_x >= GAME_WIDTH) {
                                                continue;
                                        }

                                        // Place the color in the grid
                                        GD->colorGrid[grid_y][grid_x] = color;
                                }
                        }
                }
        }

        GD->currentTetromino = GD->nextTetromino;
        GD->currentTetromino.velY = 0;
        GD->currentTetromino.x = 0;
        GD->currentTetromino.y = 0;
        SDL_Rect rect = TetrominoBounds(&GD->currentTetromino);
        GD->currentTetromino.x = GAME_POS_X + (GAME_WIDTH - rect.w) * 0.5f - rect.x;
        GD->currentTetromino.y = GAME_POS_Y - rect.h;

        // Initialize new next tetromino
        InitializeTetriminoData(&GD->tetrominoCollection, &GD->nextTetromino);
        rect = TetrominoBounds(&GD->nextTetromino);
        GD->nextTetromino.x = INFO_PANEL_X + (INFO_PANEL_WIDTH - rect.w) * 0.5f - rect.x;
        GD->nextTetromino.y = INFO_PANEL_Y + (INFO_PANEL_HEIGHT) * 0.1f;
}
