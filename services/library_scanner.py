#!/usr/bin/env python3

"""
library_scanner.py
Scans the assets/ directory for music files and a roms/ directory for ROM files.
Outputs a JSON manifest that the C app reads at startup.

Usage:
    python3 library_scanner.py
    python3 library_scanner.py --music-dir ../assets/sounds --rom-dir ../roms --out ../assets/library.json
"""

import os
import json
import argparse
import hashlib
from pathlib import Path

# ----------------------------------------------------------------
# Supported formats
# ----------------------------------------------------------------

MUSIC_EXTENSIONS = {".mp3", ".ogg", ".wav", ".flac", ".opus"}

ROM_EXTENSIONS = {
    ".nes",               # NES
    ".gb", ".gbc",        # Game Boy / Game Boy Color
    ".gba",               # Game Boy Advance
    ".smc", ".sfc",       # SNES
    ".n64", ".z64",       # N64
    ".md", ".gen",        # Mega Drive / Genesis
}

# Maps extension to a human-readable system name
ROM_SYSTEM_MAP = {
    ".nes":        "NES",
    ".gb":         "Game Boy",
    ".gbc":        "Game Boy Color",
    ".gba":        "Game Boy Advance",
    ".smc":        "SNES",
    ".sfc":        "SNES",
    ".n64":        "Nintendo 64",
    ".z64":        "Nintendo 64",
    ".md":         "Mega Drive",
    ".gen":        "Mega Drive",
}

# ----------------------------------------------------------------
# Helpers
# ----------------------------------------------------------------

def file_hash(path: Path) -> str:
    """MD5 of the first 64KB — fast fingerprint without reading the whole file."""
    h = hashlib.md5()
    with open(path, "rb") as f:
        h.update(f.read(65536))
    return h.hexdigest()


def scan_music(music_dir: Path) -> list[dict]:
    entries = []

    if not music_dir.exists():
        print(f"[scanner] Music dir not found: {music_dir}")
        return entries

    for path in sorted(music_dir.rglob("*")):
        if path.suffix.lower() not in MUSIC_EXTENSIONS:
            continue
        entries.append({
            "type":     "music",
            "title":    path.stem.replace("_", " ").title(),
            "file":     str(path),
            "format":   path.suffix.lstrip(".").upper(),
            "hash":     file_hash(path),
        })
        print(f"[scanner] music  → {path.name}")

    return entries


def scan_roms(rom_dir: Path) -> list[dict]:
    entries = []

    if not rom_dir.exists():
        print(f"[scanner] ROM dir not found: {rom_dir}")
        return entries

    for path in sorted(rom_dir.rglob("*")):
        ext = path.suffix.lower()
        if ext not in ROM_EXTENSIONS:
            continue
        entries.append({
            "type":     "rom",
            "title":    path.stem.replace("_", " ").title(),
            "file":     str(path),
            "system":   ROM_SYSTEM_MAP.get(ext, "Unknown"),
            "hash":     file_hash(path),
        })
        print(f"[scanner] rom    → {path.name}  ({ROM_SYSTEM_MAP.get(ext, '?')})")

    return entries


# ----------------------------------------------------------------
# Main
# ----------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description="MYOS library scanner")
    parser.add_argument("--music-dir", default="../assets/sounds",  help="Path to music folder")
    parser.add_argument("--rom-dir",   default="../roms",            help="Path to ROMs folder")
    parser.add_argument("--out",       default="../assets/library.json", help="Output JSON path")
    args = parser.parse_args()

    music_dir = Path(args.music_dir)
    rom_dir   = Path(args.rom_dir)
    out_path  = Path(args.out)

    print(f"[scanner] Scanning music: {music_dir}")
    print(f"[scanner] Scanning ROMs:  {rom_dir}")

    music = scan_music(music_dir)
    roms  = scan_roms(rom_dir)

    library = {
        "music": music,
        "roms":  roms,
    }

    out_path.parent.mkdir(parents=True, exist_ok=True)
    with open(out_path, "w") as f:
        json.dump(library, f, indent=2)

    print(f"\n[scanner] Done — {len(music)} music, {len(roms)} ROMs → {out_path}")


if __name__ == "__main__":
    main()