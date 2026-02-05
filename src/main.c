#include "game.h"
#include "config.h"
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>

int main(void) {
        GameContext GC;
        if (!game_init(&GC)) {
                return 1;
        }

        const Uint32 frame_delay = (Uint32)(1000.0f / TARGET_FPS);
        do {
                // Delta Time Calculation
                Uint32 current_time = SDL_GetTicks();
                Uint32 frame_time = current_time - GC.last_time;

                GC.last_time = current_time;
                GC.delta_time = frame_time / 1000.0f; // Convert to seconds

                game_handle_events(&GC);
                game_update(&GC);
                game_render(&GC);

                // Frame limiting
                Uint32 render_time = SDL_GetTicks() - current_time;
                if (render_time < frame_delay) {
                        SDL_Delay(frame_delay - render_time);
                }

                if (DEBUG) {
                        static Uint32 fps_timer = 0;
                        static int frame_count = 0;

                        frame_count++;
                        if (current_time - fps_timer >= 1000) {
                                printf("FPS: %d, Delta: %.3fms, Score: %d, Level: %d\n", frame_count, GC.delta_time * 1000.0f, GC.gameData.score, GC.gameData.level);
                                fflush(stdout);

                                frame_count = 0;
                                fps_timer = current_time;
                        }
                }
        } while (GC.running);

        game_cleanup(&GC);
        return 0;
}