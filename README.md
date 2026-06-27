# MYOS

A custom media OS built with SDL2.

## Requirements

Install the following before running:

```bash
brew install sdl2 sdl2_ttf sdl2_mixer mgba
brew install python3
pip install psutil
```

- A `.ttf` font placed at `assets/fonts/myfont.ttf`
- A background image placed at `assets/icons/fun_times.bmp`

cJSON is fetched automatically on first build — no manual install needed.

## Running

```bash
chmod +x start.sh
./start.sh
```

This will scan your library, build, and launch the app in one step.

## Adding Content

- Music: drop files into `assets/sounds/` — supported formats: mp3, ogg, wav, flac, opus
- ROMs: drop files into `assets/roms/` — supported formats: .gba, .nes, .gb, .gbc, .smc

## Compatibility

- macOS
- Linux (`sudo apt install libsdl2-dev libsdl2-ttf-dev libsdl2-mixer-dev libmgba-dev`)
- Windows support coming soon

## Controls

- Mouse to navigate menus
- Escape to go back to the previous menu
- Q to quit
- GBA controls: arrows to move, Z = B, X = A, A = L, S = R, Enter = Start, Backspace = Select