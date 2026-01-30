#ifndef CONFIG_H
#define CONFIG_H

#define DEBUG 1
#define TARGET_FPS 60

#define SCALE_FACTOR 1

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

// TODO: Better name cause this shit confusing!
#define PARTICLE_COUNT_IN_BLOCK_COLUMN (10 * SCALE_FACTOR)
#define PARTICLE_COUNT_IN_BLOCK_ROW PARTICLE_COUNT_IN_BLOCK_COLUMN

#define GRAVITY 9.8f
#define TETRIMINO_MOVE_SPEED 150
#define TIME_FOR_SAND_DELETION 1.0f

#endif