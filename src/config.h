#ifndef CONFIG_H
#define CONFIG_H

#define DEBUG 1
#define TARGET_FPS 90
#define SCALE_FACTOR 1

#define SAND_STEP_TIME (1.0f / 45.0f) / SCALE_FACTOR // Define how much update in sand per frame

// TODO: Change to setting.conf file to allow for change depending on display or just set it based on best for current display
#define VIRTUAL_WIDTH 320 * SCALE_FACTOR
#define VIRTUAL_HEIGHT 240 * SCALE_FACTOR

// Why this? Virtual * 4 (Magic Number!)
#define WINDOW_WIDTH (VIRTUAL_WIDTH * 4)
#define WINDOW_HEIGHT (VIRTUAL_HEIGHT * 4)

#define GAME_PADDING (10 * SCALE_FACTOR)
#define GAME_POS_X GAME_PADDING
#define GAME_POS_Y GAME_PADDING
#define GAME_WIDTH (int) (((2 * VIRTUAL_WIDTH) / 3) - 2 * GAME_PADDING)
#define GAME_HEIGHT (int) (VIRTUAL_HEIGHT - 2 * GAME_PADDING)

#define PARTICLE_COUNT_IN_BLOCK_COLUMN (13 * SCALE_FACTOR)
#define PARTICLE_COUNT_IN_BLOCK_ROW PARTICLE_COUNT_IN_BLOCK_COLUMN

#define INFO_PANEL_X (GAME_POS_X + GAME_WIDTH + GAME_PADDING * 2)
#define INFO_PANEL_Y GAME_POS_Y
#define INFO_PANEL_WIDTH (VIRTUAL_WIDTH - INFO_PANEL_X - GAME_PADDING)
#define INFO_PANEL_HEIGHT (VIRTUAL_HEIGHT - INFO_PANEL_Y - GAME_PADDING)

#define GRAVITY 9.8f
#define TETRIMINO_MOVE_SPEED 150 * SCALE_FACTOR
#define TIME_FOR_SAND_DELETION 0.5f

#define BASE_FONT_SIZE 124

#define FONT_PATH "./assets/Fonts/Comfortaa.ttf"

#endif