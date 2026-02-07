#ifndef AUDIO_H
#define AUDIO_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// Audio Specific
#define BG_MUSIC "./assets/Audio/Music/Bg_music.mp3"
#define MAX_CACHED_MUSIC 1

#define SFX_COUNTER_SOUND "./assets/Audio/SFX/Counter_Sound.wav"
#define SFX_GAME_OVER "./assets/Audio/SFX/Game_Over.wav"
#define SFX_LEVEL_UP "./assets/Audio/SFX/Level_Up.wav"
#define SFX_SAND_CLEAR "./assets/Audio/SFX/Sand_clear.wav"
#define MAX_CACHED_SFX 4

// Audio constants
// 0-128
#define DEFAULT_MUSIC_VOLUME 10
#define DEFAULT_SFX_VOLUME 80

// Structure to hold cached audio
typedef struct CachedAudio {
        char* path;
        Mix_Chunk* sfx;
        Mix_Music* music;
        int is_music;
        struct CachedAudio* next;
} CachedAudio;

typedef struct {
        int music_volume;
        int sfx_volume;

        CachedAudio* audio_cache;
        int is_initialized;
} AudioData;

int audio_init(AudioData* audio);
void audio_playMusic(AudioData* audio, const char* path);
void audio_playSFX(AudioData* audio, const char* path);
void audio_stopMusic(AudioData* audio);
void audio_setMusicVolume(AudioData* audio, int volume);
void audio_setSFXVolume(AudioData* audio, int volume);
void audio_cleanup(AudioData* audio);

#endif