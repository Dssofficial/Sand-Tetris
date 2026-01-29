#ifndef CONFIG_H
#define CONFIG_H

#define DEBUG 1
#define TARGET_FPS 60

// Why this? Virtual * 4 (Magic Number!)
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 960

// TODO: Change to setting.conf file to allow for change depending on display or just set it based on best for current display
#define VIRTUAL_WIDTH 320
#define VIRTUAL_HEIGHT 240

#define GAME_PADDING 5
#define GAME_POS_X GAME_PADDING
#define GAME_POS_Y GAME_PADDING
#define GAME_WIDTH (int) (((2 * VIRTUAL_WIDTH) / 3) - 2 * GAME_PADDING)
#define GAME_HEIGHT (int) (VIRTUAL_HEIGHT - 2 * GAME_PADDING)

// TODO: Better name cause this shit confusing!
#define PARTICLE_COUNT_IN_BLOCK_COLUMN 10
#define PARTICLE_COUNT_IN_BLOCK_ROW PARTICLE_COUNT_IN_BLOCK_COLUMN

#define GRAVITY 9.8f
#define TETRIMINO_MOVE_SPEED 150

#endif