#ifndef FONT_H
#define FONT_H

#include <SDL2/SDL_ttf.h>
#include <stdint.h>

typedef struct {
        TTF_Font *font;
        const char *path;

        int fontSize;
        uint8_t style;
} FontEntry;

typedef struct {
        SDL_Texture *texture;
        int w, h;

        char text[256];
        const char *fontPath;
        int fontSize;
        uint8_t style;
        SDL_Color color;
} CachedText;

typedef struct {
        CachedText *cachedTexts;
        int ct_count;
        int ct_capacity;

        FontEntry *fontEntries;
        int fe_count;
        int fe_capacity;
} FontData;


// Font Specific Functions
int fontData_init(FontData *);
void font_render_rect(FontData *, SDL_Renderer *, const char *txt, const char *font_path, int fontSize, uint8_t fontStyle, SDL_Color txtColor, SDL_Rect txtContainer);
void fontData_destroy(FontData *);

#endif