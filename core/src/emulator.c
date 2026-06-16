#include "emulator.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

static struct mCore *s_core = NULL;
static SDL_AudioDeviceID s_audio = 0;
static void *s_video_buf = NULL;

static void emulator_audio_init(struct mCore *core) {
    SDL_AudioSpec want, got;
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 512;
    want.callback = NULL;

    s_audio = SDL_OpenAudioDevice(NULL, 0, &want, &got, 0);
    SDL_PauseAudioDevice(s_audio, 0);

    blip_t *left = core->getAudioChannel(core, 0);
    blip_t *right = core->getAudioChannel(core, 1);
    blip_set_rates(left, core->frequency(core), 44100);
    blip_set_rates(right, core->frequency(core), 44100);
}

static void emulator_audio_pump(struct mCore *core) {
    blip_t *left = core->getAudioChannel(core, 0);
    blip_t *right = core->getAudioChannel(core, 1);

    int available = blip_samples_avail(left);
    if (available > 0) {
        int16_t buf[2048];
        int count = blip_read_samples(left, buf, available < 512 ? available : 512, true);
        blip_read_samples(right, buf + 1, count, true);
        SDL_QueueAudio(s_audio, buf, count * sizeof(int16_t) * 2);
    }
}

static uint32_t emulator_poll_input(void) {
    uint32_t keys = 0;
    const Uint8 *ks = SDL_GetKeyboardState(NULL);
    if (ks[SDL_SCANCODE_X]) keys |= 0x001; // A
    if (ks[SDL_SCANCODE_Z]) keys |= 0x002; // B
    if (ks[SDL_SCANCODE_BACKSPACE]) keys |= 0x004; // Select
    if (ks[SDL_SCANCODE_RETURN]) keys |= 0x008; // Start
    if (ks[SDL_SCANCODE_RIGHT]) keys |= 0x010;
    if (ks[SDL_SCANCODE_LEFT]) keys |= 0x020;
    if (ks[SDL_SCANCODE_UP]) keys |= 0x040;
    if (ks[SDL_SCANCODE_DOWN]) keys |= 0x080;
    if (ks[SDL_SCANCODE_S]) keys |= 0x100; // R
    if (ks[SDL_SCANCODE_A]) keys |= 0x200; // L
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
    mCoreLoadConfig(s_core);

    s_core->loadROM(s_core, vf);
    s_core->reset(s_core);

    emulator_audio_init(s_core);

    SDL_Texture *tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING, width, height);

    char state_path[512];
    snprintf(state_path, sizeof(state_path), "../assets/saves/%s.state",
        rom_path + (int)(strrchr(rom_path, '/') - rom_path + 1));
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

    SDL_DestroyTexture(tex);
    SDL_CloseAudioDevice(s_audio);
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