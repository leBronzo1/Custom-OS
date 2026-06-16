#include "emulator.h"
#include "audio.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

/* Dedicated mixer channel reserved for emulator audio */
#define EMU_AUDIO_CHANNEL 15

static struct mCore *s_core     = NULL;
static void         *s_video_buf = NULL;

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
    blip_set_rates(left,  core->frequency(core), 44100);
    blip_set_rates(right, core->frequency(core), 44100);
}

static void emulator_audio_pump(struct mCore *core) {
    blip_t *left  = core->getAudioChannel(core, 0);
    blip_t *right = core->getAudioChannel(core, 1);

    int available = blip_samples_avail(left);
    if (available <= 0) return;

    int count = available < 512 ? available : 512;

    /* Read each channel separately (stereo=0), then interleave manually */
    int16_t lbuf[512];
    int16_t rbuf[512];
    count = blip_read_samples(left,  lbuf, count, 0);
             blip_read_samples(right, rbuf, count, 0);

    int16_t interleaved[1024];
    for (int i = 0; i < count; i++) {
        interleaved[i * 2]     = lbuf[i];
        interleaved[i * 2 + 1] = rbuf[i];
    }

    int byte_len = count * 2 * (int)sizeof(int16_t);
    Mix_Chunk chunk = {
        .allocated = 0,
        .abuf      = (Uint8 *)interleaved,
        .alen      = (Uint32)byte_len,
        .volume    = MIX_MAX_VOLUME
    };

    Mix_HaltChannel(EMU_AUDIO_CHANNEL);
    Mix_PlayChannel(EMU_AUDIO_CHANNEL, &chunk, 0);

    /* Wait for mixer to finish with our stack buffer before returning */
    while (Mix_Playing(EMU_AUDIO_CHANNEL)) {
        SDL_Delay(1);
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
    SDL_DestroyTexture(tex);
    free(s_video_buf);
    s_video_buf = NULL;
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