# MYOS

A custom media OS built with SDL2.

## Requirements

Install the following before running:

```bash
brew install sdl2 sdl2_ttf sdl2_mixer mgba python3
```

On Debian/Ubuntu Linux:

```bash
sudo apt install build-essential curl python3-dev \
  libsdl2-dev libsdl2-ttf-dev libsdl2-mixer-dev libmgba-dev
```

### Python environment (important)

`myos` embeds a Python interpreter at build time (linked via `python3-config`)
to run `services/system_monitor.py`. Because of this, the virtual environment
**must** be created with the exact same Python version the binary is linked
against, or third-party packages like `psutil` will fail to import even
though they're installed, compiled extensions are tied to a specific
Python ABI/version.

Check which version your build links against:

```bash
python3-config --cflags
```

Then create the venv with that same version (adjust `python3.14` below to
match):

```bash
python3.14 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

If you ever upgrade your system Python or rebuild against a different
version, delete and recreate `.venv` to match.

cJSON is fetched automatically on first build — no manual install needed.

### Assets

- A `.ttf` font placed at `assets/fonts/myfont.ttf`
- A background image placed at `assets/icons/fun_times.bmp`

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
- Linux
- Windows support coming soon

## Controls

| Key | Action |
|-----|--------|
| Mouse | Navigate menus |
| Escape | Back to previous menu |
| Q | Quit |

### GBA Controls

| Key | Action |
|-----|--------|
| Arrow keys | Move |
| Z | B |
| X | A |
| A | L |
| S | R |
| Enter | Start |
| Backspace | Select |
