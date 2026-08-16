#ifndef AUDIO_H
#define AUDIO_H

#include <SDL_mixer.h>

// Holds a sound effect
typedef struct {
    Mix_Chunk *chunk;
    char name[256];
} SFX;

// Holds a music track
typedef struct {
    Mix_Music *music;
    char name[256];
} MusicTrack;

// Call once after SDL_Init() — frequency: 44100, channels: 1 (mono) 2 (stereo), chunksize: 512/1024/2048
int audio_init(int frequency, int channels, int chunksize);
 
// Shutdown and free all mixer resources
void audio_shutdown(void);

// Release SDL_mixer's audio device while the emulator owns SDL audio.
int audio_suspend_for_emulator(void);

// Reopen SDL_mixer's audio device after the emulator exits.
int audio_resume_after_emulator(void);
 
// Load a sound effect from a file path, name it for debugging — caller must free with audio_free_sfx()
SFX *audio_load_sfx(const char *path, const char *name);
 
// Free a loaded sound effect
void audio_free_sfx(SFX *sfx);
 
// Play a sound effect once at given volume (0–128)
int audio_play_sfx(SFX *sfx, int volume);
 
// Play a sound effect with loops — 0 = once, n = n extra repeats, -1 = forever
int audio_play_sfx_loop(SFX *sfx, int volume, int loops);
 
// Load a music track from a file path, name it for debugging — caller must free with audio_free_music()
MusicTrack *audio_load_music(const char *path, const char *name);
 
// Free a loaded music track
void audio_free_music(MusicTrack *track);
 
// Play music — loops: -1 = forever, 0 = once, n = n times — fade_ms: 0 = instant
void audio_play_music(MusicTrack *track, int loops, int fade_ms);
 
// Stop music — fade_ms: 0 = instant
void audio_stop_music(int fade_ms);
 
// Pause music
void audio_pause_music(void);
 
// Resume paused music
void audio_resume_music(void);
 
// Returns 1 if music is currently playing
int audio_music_playing(void);
 
// Set volume for all sound effect channels (0–128)
void audio_set_sfx_volume(int volume);
 
// Set volume for music (0–128)
void audio_set_music_volume(int volume);

// Load and immediately play a music track — caller must free with audio_free_music()
MusicTrack *audio_play_music_file(const char *path, const char *name, int loops, int fade_ms);

// Load and immediately play a sound effect — caller must free with audio_free_sfx()
SFX *audio_play_sfx_file(const char *path, const char *name, int volume);

#endif
