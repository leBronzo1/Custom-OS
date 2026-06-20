#include "emulator.h"
#include "audio.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

/* Dedicated mixer channel reserved for emulator audio */
#define EMU_AUDIO_CHANNEL 15
#define AUDIO_BUFFER_SIZE 4096

static struct mCore *s_core       = NULL;
static void         *s_video_buf  = NULL;
static int16_t      *s_audio_buf  = NULL;
static Mix_Chunk    *s_audio_chunk = NULL;

static void emulator_audio_init(struct mCore *core) {
    /*
     * Enable audio in mGBA config BEFORE reset so the audio hardware
     * initialises with the correct sample rate. Calling this after
     * reset causes blip to produce silence (all zeros).
     */
    mCoreConfigSetIntValue(&core->config, "audio.enable", 1);
    mCoreConfigSetIntValue(&core->config, "audio.volume", 256);
    mCoreLoadConfig(core);

    blip_t *left  = core->getAudioChannel(core, 0);
    blip_t *right = core->getAudioChannel(core, 1);

    /* GBA audio clock is fixed at 32768 Hz, output to 44100 Hz */
    blip_set_rates(left,  32768, 44100);
    blip_set_rates(right, 32768, 44100);

    if (!s_audio_buf) {
        s_audio_buf = malloc(AUDIO_BUFFER_SIZE * sizeof(int16_t));
    }
}

static void emulator_audio_pump(struct mCore *core) {
    static int frame_count = 0;
    blip_t *left  = core->getAudioChannel(core, 0);
    blip_t *right = core->getAudioChannel(core, 1);

    int available = blip_samples_avail(left);

    if (frame_count++ % 60 == 0) {
        printf("[audio] available samples: %d\n", available);
    }

    if (available <= 0) return;

    int count = available < 512 ? available : 512;
    if (count * 2 > AUDIO_BUFFER_SIZE) count = AUDIO_BUFFER_SIZE / 2;

    int16_t lbuf[512];
    int16_t rbuf[512];
    count = blip_read_samples(left,  lbuf, count, 0);
             blip_read_samples(right, rbuf, count, 0);

    for (int i = 0; i < count; i++) {
        s_audio_buf[i * 2]     = lbuf[i];
        s_audio_buf[i * 2 + 1] = rbuf[i];
    }

    int byte_len = count * 2 * (int)sizeof(int16_t);

    if (s_audio_chunk) Mix_FreeChunk(s_audio_chunk);
    s_audio_chunk = Mix_QuickLoad_RAW((Uint8 *)s_audio_buf, byte_len);

    if (s_audio_chunk) {
        Mix_PlayChannel(EMU_AUDIO_CHANNEL, s_audio_chunk, 0);
        if (frame_count % 60 == 1) {
            printf("[audio] Playing %d bytes\n", byte_len);
        }
    }
}

static uint32_t emulator_poll_input(void) {
    uint32_t keys = 0;
    const Uint8 *ks = SDL_GetKeyboardState(NULL);
    if (ks[SDL_SCANCODE_X])         keys |= 0x001; // A
    if (ks[SDL_SCANCODE_Z])         keys |= 0x002; // B
    if (ks[SDL_SCANCODE_BACKSPACE]) keys |= 0x004; // Select
    if (ks[SDL_SCANCODE_RETURN])    keys |= 0x008; // Start
    if (ks[SDL_SCANCODE_RIGHT])     keys |= 0x010;
    if (ks[SDL_SCANCODE_LEFT])      keys |= 0x020;
    if (ks[SDL_SCANCODE_UP])        keys |= 0x040;
    if (ks[SDL_SCANCODE_DOWN])      keys |= 0x080;
    if (ks[SDL_SCANCODE_S])         keys |= 0x100; // R
    if (ks[SDL_SCANCODE_A])         keys |= 0x200; // L
    return keys;
}

int emulator_run(SDL_Renderer *ren, const char *rom_path) {
    struct VFile *vf = VFileOpen(rom_path, O_RDONLY);
    if (!vf) { printf("[emu] Failed to open ROM: %s\n", rom_path); return -1; }

    s_core = mCoreFindVF(vf);
    if (!s_core) { printf("[emu] No core found for ROM: %s\n", rom_path); return -1; }

    s_core->init(s_core);
    mCoreInitConfig(s_core, NULL);

    unsigned width, height;
    s_core->desiredVideoDimensions(s_core, &width, &height);

    s_video_buf = malloc(width * height * 4);
    s_core->setVideoBuffer(s_core, s_video_buf, width);

    mCoreConfigSetValue(&s_core->config, "savegamePath", "../assets/saves/");

    s_core->loadROM(s_core, vf);

    /*
     * Initialize SDL mixer BEFORE audio_init — mixer must be open
     * before blip buffers try to output samples.
     */
    if (audio_init(44100, 2, 512) < 0) {
        printf("[emu] Failed to initialize audio mixer\n");
        return -1;
    }

    /*
     * Audio init MUST come before reset — blip rates need to be set
     * before the core initialises its audio hardware on reset.
     */
    emulator_audio_init(s_core);
    s_core->reset(s_core);

    SDL_Texture *tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING, width, height);

    /* Fixed state path — original code crashed if no '/' in rom_path */
    const char *basename = strrchr(rom_path, '/');
    basename = basename ? basename + 1 : rom_path;
    char state_path[512];
    snprintf(state_path, sizeof(state_path), "../assets/saves/%s.state", basename);
    emulator_load_state(state_path);

    /* Reconfigure audio after loading state — state restore can override settings */
    emulator_audio_init(s_core);

    SDL_Event e;
    int running = 1;

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) running = 0;
        }

        s_core->setKeys(s_core, emulator_poll_input());
        s_core->runFrame(s_core);

        emulator_audio_pump(s_core);

        SDL_UpdateTexture(tex, NULL, s_video_buf, width * 4);
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, NULL, NULL);
        SDL_RenderPresent(ren);
    }

    emulator_save_state(state_path);

    Mix_HaltChannel(EMU_AUDIO_CHANNEL);
    if (s_audio_chunk) Mix_FreeChunk(s_audio_chunk);
    audio_shutdown();
    SDL_DestroyTexture(tex);
    free(s_video_buf);
    free(s_audio_buf);
    s_video_buf = NULL;
    s_audio_buf = NULL;
    s_audio_chunk = NULL;
    s_core->deinit(s_core);
    s_core = NULL;

    return 0;
}

void emulator_save_state(const char *path) {
    if (!s_core) return;
    struct VFile *vf = VFileOpen(path, O_WRONLY | O_CREAT | O_TRUNC);
    if (!vf) { printf("[emu] Failed to open state for saving: %s\n", path); return; }
    mCoreSaveStateNamed(s_core, vf, SAVESTATE_SCREENSHOT);
    vf->close(vf);
    printf("[emu] State saved: %s\n", path);
}

void emulator_load_state(const char *path) {
    if (!s_core) return;
    struct VFile *vf = VFileOpen(path, O_RDONLY);
    if (!vf) return;
    mCoreLoadStateNamed(s_core, vf, SAVESTATE_SCREENSHOT);
    vf->close(vf);
    printf("[emu] State loaded: %s\n", path);
}