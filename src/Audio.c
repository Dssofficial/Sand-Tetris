#include "Audio.h"
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_stdinc.h>
#include <stdio.h>

// Initialize audio system
static void audio_preload(AudioData* audio);
int audio_init(AudioData* audio) {
        if (audio == NULL) {
                return -1;
        }

        // Initialize SDL_mixer
        if (Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 2048) < 0) {
                fprintf(stderr, "SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
                return 0;
        }

        // Initialize audio data
        audio->audio_cache = NULL;
        audio->music_volume = DEFAULT_MUSIC_VOLUME;
        audio->sfx_volume = DEFAULT_SFX_VOLUME;
        audio->is_initialized = 1;

        // Set initial volumes
        Mix_VolumeMusic(audio->music_volume);
        Mix_Volume(-1, audio->sfx_volume);  // -1 affects all channels

        // Preload
        audio_preload(audio);
        return 0;
}

// Find cached audio by path and type
static CachedAudio* find_cached_audio(AudioData* audio, const char* path, int is_music) {
        CachedAudio* current = audio->audio_cache;

        while (current != NULL) {
                if (strcmp(current->path, path) == 0 && current->is_music == is_music) {
                        return current;
                }
                current = current->next;
        }

        return NULL;
}

static void add_to_cache(AudioData* audio, const char* path, void* audio_data, int is_music) {
        if (audio == NULL || path == NULL || audio_data == NULL) {
                return;
        }

        // Check if already cached
        if (find_cached_audio(audio, path, is_music) != NULL) {
                return;
        }

        // Create new cache entry
        CachedAudio* new_entry = (CachedAudio*)malloc(sizeof(CachedAudio));
        if (new_entry == NULL) {
                printf("Failed to allocate memory for audio cache\n");
                return;
        }

        // Duplicate path
        new_entry->path = strdup(path);
        new_entry->is_music = is_music;

        // Store audio data
        if (is_music) {
                new_entry->music = (Mix_Music*)audio_data;
                new_entry->sfx = NULL;
        } else {
                new_entry->sfx = (Mix_Chunk*)audio_data;
                new_entry->music = NULL;
        }

        // Add to beginning of cache list
        new_entry->next = audio->audio_cache;
        audio->audio_cache = new_entry;
}

void audio_playMusic(AudioData* audio, const char* path) {
        if (audio->music_volume == 0) return;

        if (audio == NULL || path == NULL || !audio->is_initialized) {
                return;
        }

        // Check cache first
        CachedAudio* cached = find_cached_audio(audio, path, 1);
        if (cached != NULL) {
                // Found in cache
                if (Mix_PlayingMusic()) {
                        Mix_HaltMusic();
                }
                Mix_PlayMusic(cached->music, -1);  // -1 for infinite loop
                Mix_VolumeMusic(audio->music_volume);
        } else {
                fprintf(stderr, "Error: non_preloaded music\n");
        }
}

void audio_playSFX(AudioData* audio, const char* path) {
        if (!audio || !audio->is_initialized || audio->sfx_volume == 0) {
                return;
        }

        CachedAudio* cached = find_cached_audio(audio, path, 0);
        if (!cached) {
                fprintf(stderr, "Error: non_preloaded SFX %s\n", path);
                return;
        }

        int channel = Mix_PlayChannel(-1, cached->sfx, 0);
        if (channel == -1) {
                return;
        }

        Mix_Volume(channel, audio->sfx_volume);
}


void audio_stopMusic(AudioData* audio) {
        if (audio == NULL || !audio->is_initialized) {
                return;
        }

        if (Mix_PlayingMusic()) {
                Mix_HaltMusic();
        }
}

// Set music volume (0-128)
void audio_setMusicVolume(AudioData* audio, int volume) {
        if (audio == NULL || !audio->is_initialized) {
                return;
        }

        volume = SDL_clamp(volume, 0, 128);
        audio->music_volume = volume;
        Mix_VolumeMusic(volume);
}

// Set SFX volume (0-128)
void audio_setSFXVolume(AudioData* audio, int volume) {
        if (audio == NULL || !audio->is_initialized) {
                return;
        }

        volume = SDL_clamp(volume, 0, 128);
        audio->sfx_volume = volume;
        Mix_Volume(-1, volume);  // -1 affects all channels
}

// Free audio cache memory
static void free_audio_cache(AudioData* audio) {
        if (audio == NULL) {
                return;
        }

        CachedAudio* current = audio->audio_cache;
        CachedAudio* next;

        while (current != NULL) {
                next = current->next;

                // Free the audio data
                if (current->is_music) {
                        if (current->music != NULL) {
                                Mix_FreeMusic(current->music);
                        }
                } else {
                        if (current->sfx != NULL) {
                                Mix_FreeChunk(current->sfx);
                        }
                }

                // Free the path string
                free(current->path);

                // Free the cache entry
                free(current);

                current = next;
        }

        audio->audio_cache = NULL;
}

// Clean up audio system
void audio_cleanup(AudioData* audio) {
        if (audio == NULL || !audio->is_initialized) {
                return;
        }

        // Stop all audio
        Mix_HaltMusic();
        Mix_HaltChannel(-1);  // -1 stops all channels

        // Free cached audio
        free_audio_cache(audio);

        // Close SDL_mixer
        Mix_CloseAudio();
        audio->is_initialized = 0;
}

static inline void preload_mp3(AudioData* audio, const char* path) {
        Mix_Music* music = Mix_LoadMUS(path);
        if (!music) {
                fprintf(stderr, "Failed to preload music %s: %s\n", path, Mix_GetError());
                return;
        }
        add_to_cache(audio, path, music, 1);
}

static inline void preload_wav(AudioData* audio, const char* path) {
        Mix_Chunk* sfx = Mix_LoadWAV(path);
        if (!sfx) {
                fprintf(stderr, "Failed to preload SFX %s: %s\n", path, Mix_GetError());
                return;
        }
        add_to_cache(audio, path, sfx, 0);
}

static void audio_preload(AudioData* audio) {
        preload_mp3(audio, BG_MUSIC);

        preload_wav(audio, SFX_COUNTER_SOUND);
        preload_wav(audio, SFX_GAME_OVER);
        preload_wav(audio, SFX_LEVEL_UP);
        preload_wav(audio, SFX_SAND_CLEAR);
}
