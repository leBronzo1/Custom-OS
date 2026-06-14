# MYOS

A custom media OS built with SDL2.

## Requirements

- SDL2
- SDL2_ttf
- SDL2_mixer
- mGBA (`brew install mgba` / `sudo apt install libmgba-dev`)
- A `.ttf` font placed at `assets/fonts/myfont.ttf`
- A background image placed at `assets/icons/fun_times.bmp`

## Running

```bash
chmod +x start.sh
./start.sh
```

## Adding Content

- Music: drop files into `assets/sounds/` — supported formats: mp3, ogg, wav, flac, opus
- ROMs: drop files into `assets/roms/` — supported formats: .gba, .nes, .gb, .gbc, .smc, .n64, .md

## Compatibility

- macOS
- Linux
- Windows support coming soon

## Controls

- Mouse to navigate menus
- Escape to go back to the previous menu
- Q to quit
- GBA controls: arrows to move, Z = B, X = A, A = L, S = R, Enter = Start, Backspace = Select
