#ifndef FONT_H
#define FONT_H

#include <SDL2/SDL_ttf.h>
#include <stdint.h>

typedef enum {
        UI_ALIGN_LEFT,
        UI_ALIGN_CENTER,
        UI_ALIGN_RIGHT
} UIAlign;

typedef struct {
        TTF_Font *font;
        const char *path;

        int fontSize;
        uint8_t style;
} FontEntry;

typedef struct {
        FontEntry *entries;

        int count;
        int capacity;
} FontData;

// Font Specific Functions
int fontData_init(FontData *);

void font_render_rect(
        FontData *data,
        SDL_Renderer *renderer,
        const char *text,
        const char *font_path,
        int fontSize,
        uint8_t fontStyle,
        SDL_Color color,
        SDL_Rect container,
        UIAlign align
);

void fontData_destroy(FontData *);

#endif