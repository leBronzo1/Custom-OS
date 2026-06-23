#!/bin/bash
 
mkdir -p assets/saves
 
echo "[myos] Scanning library..."
python3 services/library_scanner.py \
    --music-dir assets/sounds \
    --rom-dir   assets/roms \
    --out       assets/library.json
 
echo "[myos] Building..."
cd core && make run 2>&1 | grep -E "\[emu|\[audio"