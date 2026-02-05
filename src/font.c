#include "font.h"
#include <SDL2/SDL_ttf.h>
#include <stdint.h>
#include <string.h>
#include "config.h"

// Font related
int fontData_init(FontData *FD) {
        FD->count = 0;
        FD->capacity = 4;
        FD->entries = malloc(sizeof(FontEntry) * FD->capacity);
        if (!FD->entries) {
                return -1;
        }
        return 0;
}

static TTF_Font* font_get(FontData *FD, const char *path, int size, uint8_t style) {
        // 1. Search
        for (int i = 0; i < FD->count; i++) {
                FontEntry *e = &FD->entries[i];

                if (
                        e->fontSize == size &&
                        e->style == style &&
                        strcmp(e->path, path) == 0
                ) {
                        return e->font;
                }
        }

        // 2. Not found -> load new font
        TTF_Font *font = TTF_OpenFont(path, size);
        if (!font) return NULL;
        TTF_SetFontStyle(font, style);

        // 3. Add to array, TODO: make it safe
        if (FD->count >= FD->capacity) {
                FD->capacity *= 2;
                FD->entries = realloc(FD->entries, sizeof(FontEntry) * FD->capacity);
        }

        FD->entries[FD->count++] = (FontEntry) {
                .font = font,
                .fontSize = size,
                .style = style,
                .path = path
        };

        return font;
}

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
) {
        if (!data || !renderer || !text) return;

        TTF_Font *font = font_get(data, font_path, fontSize == -1? BASE_FONT_SIZE: fontSize, fontStyle);
        if (!font) return;

        SDL_Surface *surface = TTF_RenderUTF8_Solid(font, text, color);
        if (!surface) return;

        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (!texture) {
                SDL_FreeSurface(surface);
                return;
        }

        int texW = surface->w;
        int texH = surface->h;
        SDL_FreeSurface(surface);

        // Scale to fit container (keep aspect ratio)
        float scaleX = (float)container.w / texW;
        float scaleY = (float)container.h / texH;
        float scale = SDL_min(scaleX, scaleY);
        scale = SDL_min(scale, 1.0f);

        int drawW = (int)(texW * scale);
        int drawH = (int)(texH * scale);

        SDL_Rect dst = {
                .w = drawW,
                .h = drawH,
                .y = container.y + (container.h - drawH) / 2
        };

        switch (align) {
                case UI_ALIGN_LEFT:
                dst.x = container.x;
                break;
                case UI_ALIGN_CENTER:
                dst.x = container.x + (container.w - drawW) / 2;
                break;
                case UI_ALIGN_RIGHT:
                dst.x = container.x + container.w - drawW;
                break;
        }

        SDL_RenderCopy(renderer, texture, NULL, &dst);
        SDL_DestroyTexture(texture);
}

void fontData_destroy(FontData *FD) {
        for (int i = 0; i < FD->count; i++) {
                TTF_CloseFont(FD->entries[i].font);
        }

        free(FD->entries);
}