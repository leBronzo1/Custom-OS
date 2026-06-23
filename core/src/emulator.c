#include "emulator.h"
#include "audio.h"
#include <mgba/core/input.h>
#include <mgba/internal/gba/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#define AUDIO_BUFFER_SIZE 4096   /* int16_t elements, interleaved L/R */
#define EMU_SAMPLE_RATE   44100
#define EMU_MAX_QUEUED_MS 100    /* cap queued audio latency/backlog */

static struct mCore      *s_core      = NULL;
static void               *s_video_buf = NULL;
static int16_t            *s_audio_buf = NULL;
static SDL_AudioDeviceID   s_audio_dev = 0;

/*
 * The emulator needs to stream continuously-generated PCM, not play
 * discrete one-shot sound effects. SDL_mixer's Mix_Chunk/Mix_PlayChannel
 * API is built for the latter: calling Mix_PlayChannel on the same
 * channel every frame restarts playback from sample 0 each time,
 * chopping audio into ~11ms fragments, and Mix_QuickLoad_RAW does not
 * copy the buffer it's given, so freeing/reusing that buffer next frame
 * races with SDL_mixer's audio thread.
 *
 * Instead we open a dedicated raw SDL audio device for the emulator and
 * feed it via SDL_QueueAudio, which is SDL's actual streaming primitive:
 * it copies the bytes into its own internal FIFO immediately and an
 * audio thread drains it continuously. This runs completely separately
 * from SDL_mixer, so there's no fighting over the audio device or a
 * single mixer channel.
 */
static void emulator_audio_init(struct mCore *core) {
    /*
     * Enable audio in mGBA config BEFORE reset so the audio hardware
     * initialises with the correct sample rate. Calling this after
     * reset causes blip to produce silence (all zeros).
     */
    mCoreConfigSetIntValue(&core->config, "audio.enable", 1);
    mCoreConfigSetIntValue(&core->config, "audio.volume", 256);
    mCoreLoadConfig(core);

    /*
     * mCoreConfigSetIntValue("audio.enable", 1) + mCoreLoadConfig does NOT
     * reliably enable the GBA's individual audio channels in this version
     * of libmgba (the 4 PSG channels + 2 direct-sound/FIFO channels) -
     * confirmed by blip_samples_avail() reporting real sample counts while
     * every sample value was 0. The actual per-channel mixer enable lives
     * behind a separate core API: enableAudioChannel().
     *
     * NOTE: we do NOT call core->listAudioChannels(core, NULL) here - passing
     * NULL as the second argument crashed (SIGBUS / bad write) inside
     * libmgba's internal list function, which appears to write through that
     * pointer unconditionally rather than treating NULL as "count only".
     * The GBA hardware always has exactly 6 audio channels (4 PSG + 2
     * direct-sound/FIFO), so we just hardcode that instead of querying it.
     */
    /*
     * TEMPORARILY DISABLED FOR DEBUGGING: enableAudioChannel calls were
     * crashing inside _GBACoreLookupIdentifier with a bad write fault.
     * lldb backtrace showed core->enableAudioChannel(core, 1, true) loading
     * a function pointer from offset 0x910 in the mCore struct and jumping
     * into a location that doesn't match the expected 3-pointer-arg
     * signature. Removing this call to confirm it's the actual crash cause
     * before re-enabling audio channels via a different approach.
     */
    /*
    for (size_t i = 0; i < 6; i++) {
        core->enableAudioChannel(core, i, true);
    }
    printf("[emu-audio] Enabled 6 audio channels\n");
    */
    printf("[emu-audio] Skipped enableAudioChannel calls (debugging)\n");

    blip_t *left  = core->getAudioChannel(core, 0);
    blip_t *right = core->getAudioChannel(core, 1);

    /* GBA audio clock is fixed at 32768 Hz, output to 44100 Hz */
    blip_set_rates(left,  32768, EMU_SAMPLE_RATE);
    blip_set_rates(right, 32768, EMU_SAMPLE_RATE);

    if (!s_audio_buf) {
        s_audio_buf = malloc(AUDIO_BUFFER_SIZE * sizeof(int16_t));
    }

    /* Open the emulator's own audio device once. We do NOT touch
       SDL_mixer here at all — the caller is responsible for stopping
       any mixer music before launching the emulator (renderer.c
       already does this via music_stop_current()). */
    if (s_audio_dev == 0) {
        SDL_AudioSpec want, have;
        SDL_zero(want);
        want.freq     = EMU_SAMPLE_RATE;
        want.format   = AUDIO_S16SYS;
        want.channels = 2;
        want.samples  = 1024;
        want.callback = NULL; /* we push samples via SDL_QueueAudio */

        s_audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
        if (s_audio_dev == 0) {
            fprintf(stderr, "[emu-audio] SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
        } else {
            SDL_PauseAudioDevice(s_audio_dev, 0); /* unpause: start draining the queue */
            printf("[emu-audio] Opened device @ %d Hz, %d channels\n", have.freq, have.channels);
        }
    }
}

static void emulator_audio_pump(struct mCore *core) {
    if (s_audio_dev == 0) return;

    blip_t *left  = core->getAudioChannel(core, 0);
    blip_t *right = core->getAudioChannel(core, 1);

    int available = blip_samples_avail(left);

    static int frame_count = 0;
    if (frame_count++ % 60 == 0) {
        printf("[emu-audio] available=%d queued_bytes=%u\n",
               available, (unsigned)SDL_GetQueuedAudioSize(s_audio_dev));
    }

    if (available <= 0) return;

    int16_t lbuf[2048];
    int16_t rbuf[2048];

    int count = available;
    if (count > AUDIO_BUFFER_SIZE / 2) count = AUDIO_BUFFER_SIZE / 2;
    if (count > 2048) count = 2048;

    count = blip_read_samples(left,  lbuf, count, 0);
             blip_read_samples(right, rbuf, count, 0);

    for (int i = 0; i < count; i++) {
        s_audio_buf[i * 2]     = lbuf[i];
        s_audio_buf[i * 2 + 1] = rbuf[i];
    }

    int byte_len = count * 2 * (int)sizeof(int16_t);

    if (frame_count % 60 == 1) {
        int16_t peak = 0;
        for (int i = 0; i < count; i++) {
            int16_t v = lbuf[i] < 0 ? -lbuf[i] : lbuf[i];
            if (v > peak) peak = v;
        }
        printf("[emu-audio] queueing %d bytes (%d samples) peak_amplitude=%d\n", byte_len, count, peak);
    }

    /* Prevent unbounded latency growth: if the emulator is running
       ahead of real time and the queue backs up, drop the backlog
       rather than letting audio drift further and further behind. */
    Uint32 queued_bytes = SDL_GetQueuedAudioSize(s_audio_dev);
    Uint32 max_bytes = (Uint32)((EMU_SAMPLE_RATE * 2 * sizeof(int16_t) * EMU_MAX_QUEUED_MS) / 1000);
    if (queued_bytes > max_bytes) {
        SDL_ClearQueuedAudio(s_audio_dev);
    }

    SDL_QueueAudio(s_audio_dev, s_audio_buf, byte_len);
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
    if (!s_core) { printf("[emu] No core found for ROM: %s\n", rom_path); return -1; }

    if (!s_core->init(s_core)) {
        printf("[emu] core->init() FAILED - core is not safely usable\n");
        s_core = NULL;
        return -1;
    }
    printf("[emu] core->init() succeeded\n");

    mCoreInitConfig(s_core, NULL);

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

    s_video_buf = malloc(width * height * 4);
    s_core->setVideoBuffer(s_core, s_video_buf, width);

    mCoreConfigSetValue(&s_core->config, "savegamePath", "../assets/saves/");

    s_core->loadROM(s_core, vf);

    /*
     * Audio init MUST come before reset — blip rates need to be set
     * before the core initialises its audio hardware on reset. This
     * also opens our dedicated raw audio device (see comment above
     * emulator_audio_init). SDL_mixer is intentionally left alone;
     * the caller already stops mixer music before invoking us.
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

    emulator_audio_shutdown();
    SDL_DestroyTexture(tex);
    free(s_video_buf);
    free(s_audio_buf);
    s_video_buf = NULL;
    s_audio_buf = NULL;
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