#include "font.h"
#include <SDL2/SDL_ttf.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

// Font related
int fontData_init(FontData *FD) {
        FD->fe_count = 0;
        FD->fe_capacity = 4;
        FD->fontEntries = malloc(sizeof(FontEntry) * FD->fe_capacity);

        FD->ct_count = 0;
        FD->ct_capacity = 8;
        FD->cachedTexts = malloc(sizeof(CachedText) * FD->ct_capacity);

        if (!FD->fontEntries || !FD->cachedTexts) {
                return -1;
        }

        return 0;
}

static TTF_Font* font_get(FontData *FD, const char *path, int size, uint8_t style) {
        // 1. Search
        for (int i = 0; i < FD->fe_count; i++) {
                FontEntry *e = &FD->fontEntries[i];

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
        if (FD->fe_count >= FD->fe_capacity) {
                FD->fe_capacity *= 2;
                FD->fontEntries = realloc(FD->fontEntries, sizeof(FontEntry) * FD->fe_capacity);
        }

        FD->fontEntries[FD->fe_count++] = (FontEntry) {
                .font = font,
                .fontSize = size,
                .style = style,
                .path = path
        };

        return font;
}

static void font_add_cache(FontData *data, SDL_Renderer *renderer, const char *text, const char *font_path, int fontSize, uint8_t style, SDL_Color color, SDL_Texture *texture, int w, int h) {
        if (data->ct_count >= data->ct_capacity) {
                data->ct_capacity *= 2;
                data->cachedTexts = realloc(data->cachedTexts, sizeof(CachedText) * data->ct_capacity);
        }

        CachedText *cache = &data->cachedTexts[data->ct_count];

        // Copy text (safe with bounds check)
        strncpy(cache->text, text, sizeof(cache->text) - 1);
        cache->text[sizeof(cache->text) - 1] = '\0';

        cache->fontPath = font_path;
        cache->fontSize = fontSize;
        cache->style = style;
        cache->color = color;
        cache->texture = texture;
        cache->w = w;
        cache->h = h;

        data->ct_count++;
}

static SDL_Texture* font_create_texture(FontData *data, SDL_Renderer *renderer, TTF_Font *font, const char *text, SDL_Color color, int *outW, int *outH) {
        SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text, color);
        if (!surface) return NULL;

        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (!texture) {
                SDL_FreeSurface(surface);
                return NULL;
        }

        *outW = surface->w;
        *outH = surface->h;
        SDL_FreeSurface(surface);

        return texture;
}

static CachedText* font_find_cache(FontData *data, const char *text, const char *font_path, int fontSize, uint8_t style, SDL_Color color) {
        for (int i = 0; i < data->ct_count; i++) {
                CachedText *cache = &data->cachedTexts[i];
                if (
                        cache->fontSize == fontSize &&
                        cache->style == style &&
                        cache->color.r == color.r &&
                        cache->color.g == color.g &&
                        cache->color.b == color.b &&
                        cache->color.a == color.a &&
                        strcmp(cache->fontPath, font_path) == 0 &&
                        strcmp(cache->text, text) == 0
                ) {
                        return cache;
                }
        }
        return NULL;
}
void font_render_rect(
        FontData *data,
        SDL_Renderer *renderer,
        const char *text,
        const char *font_path,
        int fontSize,
        uint8_t fontStyle,
        SDL_Color color,
        SDL_Rect container
) {
        if (!data || !renderer || !text) return;

        // Use base font size if -1
        int actualSize = (fontSize == -1) ? BASE_FONT_SIZE : fontSize;

        // 1. Check cache first
        CachedText *cache = font_find_cache(data, text, font_path, actualSize, fontStyle, color);

        if (cache) {
                // Cache hit - use cached texture
                int texW = cache->w;
                int texH = cache->h;

                // Scale to fit container (keep aspect ratio)
                float scaleX = (float)container.w / texW;
                float scaleY = (float)container.h / texH;
                float scale = SDL_min(scaleX, scaleY);
                scale = SDL_min(scale, 1.0f);

                int drawW = (int)(texW * scale);
                int drawH = (int)(texH * scale);

                SDL_Rect dst = {
                        .x = container.x + (container.w - drawW) / 2,
                        .y = container.y + (container.h - drawH) / 2,
                        .w = drawW,
                        .h = drawH
                };

                SDL_RenderCopy(renderer, cache->texture, NULL, &dst);
                return;
        }

        // 2. Cache miss - create new texture
        TTF_Font *font = font_get(data, font_path, actualSize, fontStyle);
        if (!font) return;

        int texW, texH;
        SDL_Texture *texture = font_create_texture(data, renderer, font, text, color, &texW, &texH);
        if (!texture) return;

        // 3. Add to cache
        font_add_cache(data, renderer, text, font_path, actualSize, fontStyle, color, texture, texW, texH);

        // 4. Render the texture
        float scaleX = (float)container.w / texW;
        float scaleY = (float)container.h / texH;
        float scale = SDL_min(scaleX, scaleY);
        scale = SDL_min(scale, 1.0f);

        int drawW = (int)(texW * scale);
        int drawH = (int)(texH * scale);

        SDL_Rect dst = {
                .x = container.x + (container.w - drawW) / 2,
                .y = container.y + (container.h - drawH) / 2,
                .w = drawW,
                .h = drawH
        };

        SDL_RenderCopy(renderer, texture, NULL, &dst);
}

void fontData_destroy(FontData *FD) {
        for (int i = 0; i < FD->fe_count; i++) {
                TTF_CloseFont(FD->fontEntries[i].font);
        }
        free(FD->fontEntries);

        for (int i = 0; i < FD->ct_count; i++) {
                if (FD->cachedTexts[i].texture) {
                        SDL_DestroyTexture(FD->cachedTexts[i].texture);
                }
        }
        free(FD->cachedTexts);
}