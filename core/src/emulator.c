#include "emulator.h"
#include <mgba/core/input.h>
#include <mgba/internal/gba/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define AUDIO_BUFFER_FRAMES 2048
#define EMU_SAMPLE_RATE 44100
#define EMU_DEVICE_FRAMES 1024
#define EMU_TARGET_QUEUED_MS 25
#define EMU_MAX_QUEUED_MS 100

static struct mCore      *s_core      = NULL;
static void               *s_video_buf = NULL;
static int16_t            *s_audio_buf = NULL;
static SDL_AudioDeviceID   s_audio_dev = 0;
static int                 s_audio_rate = EMU_SAMPLE_RATE;

/*
 * The emulator needs to stream continuously-generated PCM, not play
 * discrete one-shot sound effects. SDL_mixer's Mix_Chunk/Mix_PlayChannel
 * API is built for the latter: calling Mix_PlayChannel on the same
 * channel every frame restarts playback from sample 0 each time,
 * chopping audio into ~11ms fragments, and Mix_QuickLoad_RAW does not
 * copy the buffer it's given, so freeing/reusing that buffer next frame
 * races with SDL_mixer's audio thread.
 *
 * Instead, the caller temporarily closes SDL_mixer and this module opens
 * the playback device exclusively. SDL_QueueAudio copies each PCM block
 * into SDL's FIFO, which its audio thread drains continuously. The device
 * is closed before control returns so SDL_mixer can reopen it.
 */
static int emulator_audio_init(struct mCore *core) {
    /* Enable the channels mGBA exposes, using their API-supplied IDs rather
       than assuming GBA channel indices. */
    const struct mCoreChannelInfo *channels = NULL;
    size_t channel_count = core->listAudioChannels(core, &channels);
    if (channels && core->enableAudioChannel) {
        for (size_t i = 0; i < channel_count; i++) {
            core->enableAudioChannel(core, channels[i].id, true);
        }
        printf("[emu-audio] Enabled %zu mGBA audio channels\n", channel_count);
    } else {
        fprintf(stderr, "[emu-audio] mGBA exposed no configurable audio channels\n");
    }

    blip_t *left  = core->getAudioChannel(core, 0);
    blip_t *right = core->getAudioChannel(core, 1);
    if (!left || !right) {
        fprintf(stderr, "[emu-audio] mGBA did not provide stereo audio buffers\n");
        return -1;
    }

    if (!s_audio_buf) {
        s_audio_buf = malloc(AUDIO_BUFFER_FRAMES * 2 * sizeof(*s_audio_buf));
        if (!s_audio_buf) {
            fprintf(stderr, "[emu-audio] Could not allocate the streaming buffer\n");
            return -1;
        }
    }

    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = EMU_SAMPLE_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = EMU_DEVICE_FRAMES;
    want.callback = NULL; /* push samples with SDL_QueueAudio */

    s_audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (s_audio_dev == 0) {
        fprintf(stderr, "[emu-audio] SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
        return -1;
    }

    s_audio_rate = have.freq;
    core->setAudioBufferSize(core, EMU_DEVICE_FRAMES);

    /* mGBA timestamps samples in emulated CPU cycles. Its official SDL
       frontend likewise uses core->frequency() as the blip input clock. */
    blip_set_rates(left, core->frequency(core), s_audio_rate);
    blip_set_rates(right, core->frequency(core), s_audio_rate);

    SDL_PauseAudioDevice(s_audio_dev, 0);
    printf("[emu-audio] Opened exclusive device @ %d Hz, %d channels\n",
           have.freq, have.channels);
    return 0;
}

static void emulator_audio_pump(struct mCore *core) {
    if (s_audio_dev == 0) return;

    blip_t *left  = core->getAudioChannel(core, 0);
    blip_t *right = core->getAudioChannel(core, 1);

    if (!left || !right || !s_audio_buf) return;

    int available = blip_samples_avail(left);
    int right_available = blip_samples_avail(right);
    if (right_available < available) available = right_available;
    if (available <= 0) return;

    int count = available;
    if (count > AUDIO_BUFFER_FRAMES) count = AUDIO_BUFFER_FRAMES;
    count = blip_read_samples(left, s_audio_buf, count, 1);
    int right_count = blip_read_samples(right, s_audio_buf + 1, count, 1);
    if (right_count < count) count = right_count;

    int byte_len = count * 2 * (int)sizeof(int16_t);

    /* Prevent unbounded latency growth: if the emulator is running
       ahead of real time and the queue backs up, drop the backlog
       rather than letting audio drift further and further behind. */
    Uint32 queued_bytes = SDL_GetQueuedAudioSize(s_audio_dev);
    Uint32 bytes_per_second = (Uint32)(s_audio_rate * 2 * sizeof(int16_t));
    Uint32 max_bytes = bytes_per_second * EMU_MAX_QUEUED_MS / 1000;
    if (queued_bytes > max_bytes) {
        SDL_ClearQueuedAudio(s_audio_dev);
    }

    if (SDL_QueueAudio(s_audio_dev, s_audio_buf, (Uint32)byte_len) < 0) {
        fprintf(stderr, "[emu-audio] SDL_QueueAudio failed: %s\n", SDL_GetError());
        return;
    }

    /* Audio is the stable clock on renderers where vsync is unavailable
       or runs above 60 Hz. Briefly wait instead of repeatedly dropping a
       growing queue, which otherwise produces fast and choppy playback. */
    queued_bytes = SDL_GetQueuedAudioSize(s_audio_dev);
    Uint32 target_bytes = bytes_per_second * EMU_TARGET_QUEUED_MS / 1000;
    if (queued_bytes > target_bytes) {
        Uint32 delay_ms = (queued_bytes - target_bytes) * 1000 / bytes_per_second;
        if (delay_ms > 0) SDL_Delay(delay_ms);
    }
}

static void emulator_audio_shutdown(void) {
    if (s_audio_dev != 0) {
        SDL_PauseAudioDevice(s_audio_dev, 1);
        SDL_ClearQueuedAudio(s_audio_dev);
        SDL_CloseAudioDevice(s_audio_dev);
        s_audio_dev = 0;
        printf("[emu-audio] Device closed.\n");
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
    if (!s_core) {
        printf("[emu] No core found for ROM: %s\n", rom_path);
        vf->close(vf);
        return -1;
    }

    if (!s_core->init(s_core)) {
        printf("[emu] core->init() FAILED - core is not safely usable\n");
        vf->close(vf);
        s_core = NULL;
        return -1;
    }
    printf("[emu] core->init() succeeded\n");

    mCoreInitConfig(s_core, NULL);
    mCoreConfigSetIntValue(&s_core->config, "volume", 0x100);
    mCoreConfigSetIntValue(&s_core->config, "mute", 0);
    mCoreConfigSetUIntValue(&s_core->config, "audioBuffers", EMU_DEVICE_FRAMES);
    mCoreConfigSetUIntValue(&s_core->config, "sampleRate", EMU_SAMPLE_RATE);
    mCoreConfigSetValue(&s_core->config, "savegamePath", "../assets/saves/");
    mCoreLoadConfig(s_core);

    /*
     * CRITICAL: mInputMapInit must be called after mCoreInitConfig.
     * Without this, core->inputMap is left as uninitialized memory.
     * Any internal mGBA code path that touches the input map (directly
     * or indirectly) reads garbage, which can manifest as a crash deep
     * inside an unrelated-looking function - this matches the
     * _GBACoreLookupIdentifier crash we were seeing on every game launch.
     */
    mInputMapInit(&s_core->inputMap, &GBAInputInfo);

    unsigned width, height;
    s_core->desiredVideoDimensions(s_core, &width, &height);

    s_video_buf = malloc(width * height * sizeof(color_t));
    if (!s_video_buf) {
        fprintf(stderr, "[emu] Could not allocate the video buffer\n");
        vf->close(vf);
        mInputMapDeinit(&s_core->inputMap);
        mCoreConfigDeinit(&s_core->config);
        s_core->deinit(s_core);
        s_core = NULL;
        return -1;
    }
    s_core->setVideoBuffer(s_core, s_video_buf, width);

    if (!s_core->loadROM(s_core, vf)) {
        fprintf(stderr, "[emu] mGBA failed to load ROM: %s\n", rom_path);
        vf->close(vf);
        free(s_video_buf);
        s_video_buf = NULL;
        mInputMapDeinit(&s_core->inputMap);
        mCoreConfigDeinit(&s_core->config);
        s_core->deinit(s_core);
        s_core = NULL;
        return -1;
    }

    /*
     * Audio init MUST come before reset — blip rates need to be set
     * before the core initialises its audio hardware on reset. This
     * also opens our dedicated raw audio device (see comment above
     * emulator_audio_init). The caller has already released SDL_mixer's
     * device before invoking us.
     */
    if (emulator_audio_init(s_core) < 0) {
        fprintf(stderr, "[emu] Continuing without emulator audio\n");
    }
    s_core->reset(s_core);

    SDL_Texture *tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!tex) {
        fprintf(stderr, "[emu] SDL_CreateTexture failed: %s\n", SDL_GetError());
        emulator_audio_shutdown();
        free(s_video_buf);
        free(s_audio_buf);
        s_video_buf = NULL;
        s_audio_buf = NULL;
        s_core->unloadROM(s_core);
        mInputMapDeinit(&s_core->inputMap);
        mCoreConfigDeinit(&s_core->config);
        s_core->deinit(s_core);
        s_core = NULL;
        return -1;
    }

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

        SDL_UpdateTexture(tex, NULL, s_video_buf, width * (int)sizeof(color_t));
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, NULL, NULL);
        SDL_RenderPresent(ren);
    }

    emulator_save_state(state_path);

    emulator_audio_shutdown();
    SDL_DestroyTexture(tex);
    free(s_video_buf);
    free(s_audio_buf);
    s_video_buf = NULL;
    s_audio_buf = NULL;
    s_core->unloadROM(s_core);
    mInputMapDeinit(&s_core->inputMap);
    mCoreConfigDeinit(&s_core->config);
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
